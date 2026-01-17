import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain
from conan.tools.env import VirtualRunEnv
from conan.tools.env.environment import _EnvVarPlaceHolder


class OsrmVirtualRunEnv(VirtualRunEnv):
    def generate(self, scope="run"):
        self._conanfile.conf.define("tools.env:dotenv", True)
        super().generate(scope)

        # generate a file that can be piped into $GITHUB_PATH like this
        # while read -r line; do echo "$line" >> "$GITHUB_PATH"; done < *runpath
        path = os.path.join(
            self._conanfile.generators_folder, self._filename + ".runpath"
        )
        with open(path, "w") as fp:
            env_vars = self.environment().vars(self._conanfile, scope=scope)
            varvalues = env_vars._values["PATH"]
            for v in varvalues._values:
                if v is not _EnvVarPlaceHolder:
                    print(v, file=fp)


class OsrmConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("boost/1.85.0")
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
        self.options["bzip2"].shared = True
        self.options["xz_utils"].shared = True

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = "20"
        tc.generate()

        ms = OsrmVirtualRunEnv(self)
        ms.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
