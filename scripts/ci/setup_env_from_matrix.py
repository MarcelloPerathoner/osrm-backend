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


def in_job_name(needle, value, default):
    """Return value if the needle was found in the job name else default."""
    haystack = job_name.split("-")
    return value if needle in haystack else default


def get(key, default=None):
    """Get a value from the job matrix with default."""
    if key in matrix:
        values[key] = matrix[key]
    elif default is not None:
        # careful: do not overwrite with None
        values[key] = default


# guess these values from the job name
# fmt: off
values["BUILD_TYPE"]         = in_job_name("debug",  "Debug", "Release")
values["BUILD_SHARED_LIBS"]  = in_job_name("shared", "ON",    "OFF")
values["BUILD_NODE_PACKAGE"] = in_job_name("node",   "ON",    "OFF")
values["ENABLE_CONAN"]       = in_job_name("conan",  "ON",    "OFF")
values["ENABLE_TIDY"]        = in_job_name("tidy",   "ON",    "OFF")
values["ENABLE_COVERAGE"]    = in_job_name("cov",    "ON",    "OFF")
values["ENABLE_ASAN"]        = in_job_name("asan",   "ON",    "OFF")
values["ENABLE_UBSAN"]       = in_job_name("ubsan",  "ON",    "OFF")

get("NODE_VERSION",       24)
get("ENABLE_ASSERTIONS",  "OFF")
get("ENABLE_COVERAGE",    values["ENABLE_COVERAGE"])
get("ENABLE_ASAN",        values["ENABLE_ASAN"])
get("ENABLE_UBSAN",       values["ENABLE_UBSAN"])
get("ENABLE_LTO",         "ON")
get("USE_CCACHE",         "ccache")

get("BUILD_UNIT_TESTS",   "ON")
get("RUN_UNIT_TESTS",     "ON")
get("RUN_CUCUMBER_TESTS", "ON")
get("RUN_NODE_TESTS",     values["BUILD_NODE_PACKAGE"])
get("BUILD_BENCHMARKS",   "OFF")
get("RUN_BENCHMARKS",     "OFF")

# fmt: on

compiler = None
version = None

# default compiler is clang
if "windows" not in matrix["runs-on"]:
    compiler = "clang"

# job name explicitly mentiones compiler
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
# explicit compiler mentioned in CC and CXX
get("CC")
get("CFLAGS")
get("CXX")
get("CXXFLAGS")
get("CLANG_TIDY")
get("LLVM")

# fmt: on

# store values for following job steps
for key in sorted(values):
    if values[key] is not None:
        print(f"{key}={values[key]}")

# { "name": "conan-clang-42-debug-shared-node-tidy-asan-ubsan-cov", "RUN_NODE_TESTS": "OFF" }
