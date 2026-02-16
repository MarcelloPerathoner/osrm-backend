"""Transforms the github matrix into a set of environment variables and cmake definitions.

stdin: the github matrix converted to json.
stdout: KEY=VALUE pairs one per line

Usage example (on github CI):

.. code: bash

   echo '${{ toJSON(matrix) }}' | python scripts/ci/setup_env_from_matrix.py >> $GITHUB_ENV
   ...
   # later step
   cmake -B build ${CMAKE_CONFIGURE_PARAMETERS}
   cmake ${CMAKE_BUILD_PARAMETERS}

"""

import json
import os
import re
import sys

matrix = json.load(sys.stdin)
job_name = matrix.get("name")

cdefs = {}  # definitions for cmake configure stage
cparams = {}  # parameters for cmake configure stage
bparams = {}  # parameters for cmake build stage
envs = {}  # environment variables


def in_job_name(needle, value, default=None):
    """Return value if the needle was found in the job name else default."""
    haystack = job_name.split("-")
    return value if needle in haystack else default


def get(d, key, default=None):
    """Get a value from the job matrix with default."""
    if key in matrix:
        d[key] = matrix[key]
    elif default is not None:
        # careful: do not overwrite existing key with None
        d[key] = default


# { "name": "conan-clang-42-debug-shared-node-tidy-asan-ubsan-cov", "RUN_NODE_TESTS": "OFF" }

# fmt: off
# encoded in job name
envs["OSRM_CONFIG"]         = in_job_name("debug",  "Debug", "Release")
cdefs["BUILD_SHARED_LIBS"]  = in_job_name("shared", "ON")
cdefs["BUILD_NODE_PACKAGE"] = in_job_name("node",   "ON")
cdefs["ENABLE_CONAN"]       = in_job_name("conan",  "ON", matrix.get("ENABLE_CONAN"))
cdefs["ENABLE_TIDY"]        = in_job_name("tidy",   "ON", matrix.get("ENABLE_TIDY"))
cdefs["ENABLE_COVERAGE"]    = in_job_name("cov",    "ON", matrix.get("ENABLE_COVERAGE"))
cdefs["ENABLE_ASAN"]        = in_job_name("asan",   "ON", matrix.get("ENABLE_ASAN"))
cdefs["ENABLE_UBSAN"]       = in_job_name("ubsan",  "ON", matrix.get("ENABLE_UBSAN"))

# not encoded in job name
get(cdefs, "ENABLE_ASSERTIONS")
get(cdefs, "ENABLE_CCACHE")
get(cdefs, "ENABLE_LTO")
get(cdefs, "ENABLE_SCCACHE")

get(envs, "NODE_VERSION",       24)
get(envs, "BUILD_UNIT_TESTS",   "ON")
get(envs, "BUILD_BENCHMARKS",   "OFF")
get(envs, "RUN_UNIT_TESTS",     "ON")
get(envs, "RUN_CUCUMBER_TESTS", "ON")
get(envs, "RUN_NODE_TESTS",     cdefs.get("BUILD_NODE_PACKAGE"))
get(envs, "RUN_BENCHMARKS",     "OFF")

# fmt: on

# In Cmake single-config generators like "Unix Makefiles" need different parameters as
# multi-config generators like "Visual Studio" and "Xcode". Currently the only
# multi-config generator is on Windows.

if "windows" in matrix["runs-on"]:
    build_dir = "build"
    bparams["--build"] = build_dir
    bparams["--config"] = envs.get("OSRM_CONFIG")
else:
    build_dir = os.path.join("build", envs.get("OSRM_CONFIG"))
    cparams["-B"] = build_dir
    cdefs["CMAKE_BUILD_TYPE"] = envs.get("OSRM_CONFIG")
    bparams["--build"] = build_dir

### Decode compiler from job name ###

compiler = None
version = None

# default compiler is clang
if "windows" not in matrix["runs-on"]:
    compiler = "clang"

# job name explicitly mentions compiler
m = re.search(r"(clang|gcc)-(\d+)", job_name)
if m:
    compiler = m.group(1)
    version = m.group(2)

envs["COMPILER_ID"] = compiler
envs["COMPILER_VERSION"] = version

ver = "-" + version if version else ""
if compiler == "clang":
    envs["CC"] = f"clang{ver}"
    envs["CXX"] = f"clang++{ver}"
    if cdefs["ENABLE_TIDY"] == "ON":
        envs["CLANG_TIDY"] = f"clang-tidy{ver}"
    if cdefs["ENABLE_COVERAGE"] == "ON":
        envs["LLVM"] = f"llvm{ver}"

if compiler == "gcc":
    envs["CC"] = f"gcc{ver}"
    envs["CXX"] = f"g++{ver}"

# fmt: off
# let the user override our choice using explicit CC, CXX etc.
get(envs, "CC")
get(envs, "CFLAGS")
get(envs, "CXX")
get(envs, "CXXFLAGS")
get(envs, "CLANG_TIDY")
get(envs, "LLVM")

cdefs["CMAKE_C_COMPILER"]     = envs.get("CC")
cdefs["CMAKE_CXX_COMPILER"]   = envs.get("CXX")
cdefs["CMAKE_CXX_CLANG_TIDY"] = envs.get("CLANG_TIDY")
cdefs["CMAKE_C_FLAGS"]        = envs.get("CFLAGS")
cdefs["CMAKE_CXX_FLAGS"]      = envs.get("CXXFLAGS")

# fmt: on

# values for piping into >> $GITHUB_ENV
envs.update(cdefs)
for key in sorted(envs):
    if envs[key] is not None:
        print(f"{key}={envs[key]}")

# Conan will write its own version of these entries
if cdefs["ENABLE_CONAN"] != "ON":

    # params for cmake -B build -DFOO=ON -DBAR=OFF
    #                  ^^^^^^^^
    params = []
    for key in sorted(cparams):
        if cparams[key] is not None:
            params.append(f"{key} {cparams[key]}")

    # definitions for cmake -B build -DFOO=ON -DBAR=OFF
    #                                ^^^^^^^^^^^^^^^^^^
    for key in sorted(cdefs):
        if cdefs[key] is not None:
            params.append(f"-D{key}={cdefs[key]}")

    print(f'CMAKE_CONFIGURE_PARAMETERS={" ".join(params)}')

    # params for cmake --build build --config Release
    #                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    params = []
    for key in sorted(bparams):
        if bparams[key] is not None:
            params.append(f"{key} {bparams[key]}")

    print(f'CMAKE_BUILD_PARAMETERS={" ".join(params)}')
