"""
Transforms the github matrix into a `CMakePresets.json` and a set of environment variables.

The idea is to produce a build environment that is similar and interchangeable with the
build environment produced by Conan. So always keep this in sync with `conanfile.py`.

See also: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html

Usage example (use this on github CI):

.. code: bash

   echo '${{ toJSON(matrix) }}' | python scripts/ci/decode_matrix.py \
        --cmake-presets-template scripts/ci/CMakePresets.template.json \
        --cmake-presets CMakePresets.json - >> $GITHUB_ENV

Usage example for the presets:

.. code: bash

   cmake --preset $CMAKE_CONFIG_PRESET_NAME
   cmake --build --preset $CMAKE_BUILD_PRESET_NAME

"""

import argparse
import json
import multiprocessing
import os
import re
import string


parser = argparse.ArgumentParser(
    description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
)

parser.add_argument(
    "matrix",
    type=argparse.FileType("r"),
    metavar="FILENAME",
    help="the github matrix as json",
)
parser.add_argument(
    "-o",
    "--output",
    type=argparse.FileType("w"),
    metavar="FILENAME",
    help="the output .env file",
)
parser.add_argument(
    "--cmake-presets-template",
    type=argparse.FileType("r"),
    metavar="FILENAME",
    help="the CMakePresets.json template file",
)
parser.add_argument(
    "--cmake-presets",
    type=argparse.FileType("w"),
    metavar="FILENAME",
    help="the generated CMakePresets.json file",
)

args = parser.parse_args()

matrix = json.load(args.matrix)
job_name = matrix.get("name")

envs = {}  # environment variables only for $GITHUB_ENV
cdefs = {}  # definitions for CmakePresets.json and $GITHUB_ENV


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
# multi-config generator we use is "Visual Studio".

config = envs["OSRM_CONFIG"]
preset_name = config.lower()

cdefs["CMAKE_BUILD_TYPE"] = config
# Conan gives two different names (on multi-config), but we do not
envs["CMAKE_CONFIGURE_PRESET_NAME"] = preset_name
envs["CMAKE_BUILD_PRESET_NAME"] = preset_name

# our "well-known" build root
binary_dir = os.path.join("${sourceDir}", "build")
if "windows" not in matrix["runs-on"]:
    binary_dir = os.path.join(binary_dir, envs["OSRM_CONFIG"])

jobs = multiprocessing.cpu_count()
envs["JOBS"] = jobs

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

# write KEY=VALUE pairs
envs.update(cdefs)
for key in sorted(envs):
    if envs[key] is not None:
        print(f"{key}={envs[key]}", file=args.output)

# Write CMakePresets.json

if args.cmake_presets_template and args.cmake_presets:
    s = string.Template(args.cmake_presets_template.read())
    s = s.substitute(name=preset_name, config=config, binary_dir=binary_dir, jobs=jobs)
    js = json.loads(s)

    cache_vars = {"CMAKE_POLICY_DEFAULT_CMP0091": "NEW"}
    for key, value in cdefs.items():
        if value is not None:
            cache_vars[key] = value

    js["configurePresets"][0]["cacheVariables"] = cache_vars
    json.dump(js, args.cmake_presets, indent=4)
