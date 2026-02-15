"""Transforms the github matrix into a set of environment variables.

stdin: the github matrix as json
stdout: KEY=VALUE pairs one per line

Usage example (on github CI):

.. code: bash

   echo '${{ toJSON(matrix) }}' | python scripts/ci/setup_env_from_matrix.py > job.env
   source job.env
   cat job.env >> $GITHUB_ENV

"""

import json
import re
import sys

matrix = json.load(sys.stdin)
values = {}
job_name = matrix.get("name")


def in_job_name(needle, value, default=None):
    """Return value if the needle was found in the job name else default."""
    haystack = job_name.split("-")
    return value if needle in haystack else default


def get(key, default=None):
    """Get a value from the job matrix with default."""
    if key in matrix:
        values[key] = matrix[key]
    elif default is not None:
        # careful: do not overwrite existing key with None
        values[key] = default


# fmt: off

# encoded in job name
values["BUILD_TYPE"]         = in_job_name("debug",  "Debug", "Release")
values["BUILD_SHARED_LIBS"]  = in_job_name("shared", "ON")
values["BUILD_NODE_PACKAGE"] = in_job_name("node",   "ON")
values["ENABLE_CONAN"]       = in_job_name("conan",  "ON", matrix.get("ENABLE_CONAN"))
values["ENABLE_TIDY"]        = in_job_name("tidy",   "ON", matrix.get("ENABLE_TIDY"))
values["ENABLE_COVERAGE"]    = in_job_name("cov",    "ON", matrix.get("ENABLE_COVERAGE"))
values["ENABLE_ASAN"]        = in_job_name("asan",   "ON", matrix.get("ENABLE_ASAN"))
values["ENABLE_UBSAN"]       = in_job_name("ubsan",  "ON", matrix.get("ENABLE_UBSAN"))

# not encoded in job name
get("ENABLE_ASSERTIONS")
get("ENABLE_LTO")
get("USE_COMPILER_CACHE")

get("NODE_VERSION",       24)
get("BUILD_UNIT_TESTS",   "ON")
get("RUN_UNIT_TESTS",     "ON")
get("RUN_CUCUMBER_TESTS", "ON")
get("RUN_NODE_TESTS",     values.get("BUILD_NODE_PACKAGE"))
get("BUILD_BENCHMARKS",   "OFF")
get("RUN_BENCHMARKS",     "OFF")

# fmt: on

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

values["COMPILER_ID"] = compiler
values["COMPILER_VERSION"] = version

ver = "-" + version if version else ""
if compiler == "clang":
    values["CC"] = f"clang{ver}"
    values["CXX"] = f"clang++{ver}"
    if values["ENABLE_TIDY"] == "ON":
        values["CLANG_TIDY"] = f"clang-tidy{ver}"
    if values["ENABLE_COVERAGE"] == "ON":
        values["LLVM"] = f"llvm{ver}"

if compiler == "gcc":
    values["CC"] = f"gcc{ver}"
    values["CXX"] = f"g++{ver}"

# fmt: off
# user may override using CC and CXX etc.
get("CC")
get("CFLAGS")
get("CXX")
get("CXXFLAGS")
get("CLANG_TIDY")
get("LLVM")

# fmt: on

# store values for following job steps
defines = []
for key in sorted(values):
    if values[key] is not None:
        print(f"{key}={values[key]}")
        if (
            key.startswith("ENABLE_")
            or key.startswith("BUILD_")
            or key in ["USE_CCACHE", "CLANG_TIDY"]
        ):
            defines.append(f"-D{key}={values[key]}")

print(f"CMAKE_DEFINITIONS={" ".join(defines)}")

# { "name": "conan-clang-42-debug-shared-node-tidy-asan-ubsan-cov", "RUN_NODE_TESTS": "OFF" }
