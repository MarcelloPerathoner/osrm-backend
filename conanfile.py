import os
import re
import subprocess
import textwrap

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.cmake.cmakedeps.cmakedeps import CMakeDeps
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.env.environment import _EnvVarPlaceHolder


class OsrmGenericBlock:
    # Everything here is copied verbatim into the toolchain file
    template = textwrap.dedent(
        """
        # Conan was here!
        """
    )

    def context(self):
        return {}


boolean_true_expressions = ("1", "true", "on")


def _getOpt(name):
    return os.environ.get(name, "").lower() in boolean_true_expressions


def _bash_path(path):
    """Return a real path even on Windows

    D:\\a\\b\\c => /d/a/b/c
    """
    m = re.match(r"^([A-Z]):(.*)$", path)
    if m:
        drive = m.group(1).lower()
        path = f"/{drive}{m.group(2)}"
    return path.replace("\\", "/")


class OsrmConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    options = {
        "shared": [True, False],
        "node_package": [True, False],
        "ccache": [False, "ccache", "sccache"],
    }

    default_options = {"shared": False, "node_package": False, "ccache": "ccache"}

    def _getVarValue(self, varvalues):
        """Returns var value as string, drops placeholders"""
        values = []
        for varvalue in varvalues._values:
            if varvalue is not _EnvVarPlaceHolder:
                values.append(_bash_path(varvalue))
        return ":".join(values)

    def _writeEnvSh(self, env_vars):
        """
        Usually Conan puts the environments for building and running into `conanbuild.sh`
        and `conanrun.sh` and you are supposed to source those files.  The troubles start
        when we run under Windows and use a bash shell, like we do on github CI.

        Setting 5 different configuration entries we could configure Conan almost but not
        quite entirely unlike the way we want.  To avoid that config hell we just write
        the file ourselves.
        """

        scope = env_vars._scope
        env_path = os.path.join(self.folders.generators_folder, f"conan-{scope}-env.sh")
        with open(env_path, "w") as fp:
            for varname, varvalues in env_vars._values.items():
                values = []
                for varvalue in varvalues._values:
                    if varvalue is _EnvVarPlaceHolder:
                        values.append(f"${varname}")
                    else:
                        values.append(_bash_path(varvalue))
                line = f"{varname}={':'.join(values)}"
                print(line, file=fp)
                self.output.info(f"scope={scope}: {line}")

    def requirements(self):
        self.requires("boost/1.88.0")
        self.requires("bzip2/1.0.8")
        self.requires("expat/2.6.2")
        self.requires("lua/5.4.6")
        self.requires("onetbb/2021.12.0")
        self.requires("xz_utils/5.4.5")
        self.requires("zlib/1.3.1")

    def configure(self):
        self.options["boost"].without_python = True
        self.options["boost"].without_coroutine = True
        self.options["boost"].without_stacktrace = True
        self.options["boost"].without_cobalt = True
        # boost::locale puts obsolete libiconv in macOS PATH
        self.options["boost"].without_locale = True

        if self.options.shared or _getOpt("BUILD_SHARED_LIBS"):
            self.options["*"].shared = True
        else:
            self.options["bzip2"].shared = True
            self.options["xz_utils"].shared = True
            self.options["hwloc"].shared = True  # required by onetbb
            if self.settings.os == "Windows":
                self.options["boost"].shared = True
                self.options["onetbb"].shared = True

    def generate(self):
        tc = CMakeToolchain(self)
        # cache_variables end up in CMakePresets.json
        # and can be recalled with `cmake --preset conan-release`
        tc.cache_variables["USE_CONAN"] = True
        tc.cache_variables["BUILD_SHARED_LIBS"] = (
            _getOpt("BUILD_SHARED_LIBS") or self.options.shared
        )
        tc.cache_variables["BUILD_NODE_PACKAGE"] = (
            _getOpt("BUILD_NODE_PACKAGE") or self.options.node_package
        )
        if "USE_CCACHE" in os.environ:
            tc.cache_variables["USE_CCACHE"] = os.environ["USE_CCACHE"]
        else:
            tc.cache_variables["USE_CCACHE"] = self.options.ccache

        # Note: this does not mean we are supporting all of these options yet in conan
        for i in (
            "ASSERTIONS",
            "CCACHE",
            "LTO",
            "SCCACHE",
            "ASAN",
            "UBSAN",
            "COVERAGE",
            "CLANG_TIDY",
        ):
            tc.cache_variables[f"ENABLE_{i}"] = _getOpt(f"ENABLE_{i}")

        # OSRM uses C++20
        # replace the block that would set the cpp standard with our own custom block
        # tc.blocks["cppstd"] = OsrmGenericBlock
        tc.blocks["generic"] = OsrmGenericBlock
        tc.generate()

        # add variable names compatible with the non-conan build
        # eg. "LUA_LIBRARIES" in addition to "lua_LIBRARIES"
        deps = CMakeDeps(self)
        deps.set_property("bzip2", "cmake_additional_variables_prefixes", ["BZIP2"])
        deps.set_property("expat", "cmake_additional_variables_prefixes", ["EXPAT"])
        deps.set_property("lua", "cmake_additional_variables_prefixes", ["LUA"])
        deps.generate()

        vre = VirtualRunEnv(self)
        vbe = VirtualBuildEnv(self)
        vre.generate()
        vbe.generate()

        self._writeEnvSh(vbe.environment().vars(self, scope="build"))
        self._writeEnvSh(vre.environment().vars(self, scope="run"))

        # Put a bootstrap environment into the well-known location `build/conan.env` to
        # aid in finding `conan-build-env.sh` and `conan-run-env.sh` since these do not
        # always end in the same place.
        os.path.join(self.recipe_folder, "build/conan.env")
        with open(os.path.join(self.recipe_folder, "build/conan.env"), "w") as fp:
            build_dir = _bash_path(self.folders.build_folder)
            generators_dir = _bash_path(self.folders.generators_folder)
            preset = f"conan-{self.settings.build_type}".lower()
            if self.settings.os == "Windows":
                preset = "conan-default"

            fp.write(f"CONAN_BUILD_DIR={build_dir}\n")
            fp.write(f"CONAN_GENERATORS_DIR={generators_dir}\n")
            fp.write(f"CONAN_CMAKE_PRESET={preset}\n")

            # HACK: Conan emits the search PATH for libraries in the "run" environment
            # but we need it during the cmake configure stage because
            # GET_RUNTIME_DEPENDENCIES() is too dumb to look into the PATH when
            # searching for DLLs on Windows.
            values = vre.environment().vars(self, scope="run")._values
            if "PATH" in values:
                conan_deps = self._getVarValue(values["PATH"])
                fp.write(f"CONAN_LIBRARY_PATH={conan_deps}\n")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()


if __name__ == "__main__":
    # run tests
    print(_bash_path("D:\\a\\b\\c"))
