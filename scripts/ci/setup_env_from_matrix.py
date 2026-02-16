"""Transforms the github matrix into a set of environment variables and cmake definitions.

stdin: the github matrix converted to json.
stdout: KEY=VALUE pairs one per line

Usage example (on github CI):

.. code: bash

   echo '${{ toJSON(matrix) }}' | python scripts/ci/setup_env_from_matrix.py > job.env
   cat job.env >> $GITHUB_ENV  # store for later
   source job.env              # use now
   cmake -B build ${MATRIX_CMAKE_DEFINITIONS}

"""

import json
import re
import sys

matrix = json.load(sys.stdin)
defs = {}  # these go into cmake -D... and into the .env file
envs = {}  # these go into the .env file only
job_name = matrix.get("name")


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
defs["BUILD_TYPE"]         = in_job_name("debug",  "Debug", "Release")
defs["BUILD_SHARED_LIBS"]  = in_job_name("shared", "ON")
defs["BUILD_NODE_PACKAGE"] = in_job_name("node",   "ON")
defs["ENABLE_CONAN"]       = in_job_name("conan",  "ON", matrix.get("ENABLE_CONAN"))
defs["ENABLE_TIDY"]        = in_job_name("tidy",   "ON", matrix.get("ENABLE_TIDY"))
defs["ENABLE_COVERAGE"]    = in_job_name("cov",    "ON", matrix.get("ENABLE_COVERAGE"))
defs["ENABLE_ASAN"]        = in_job_name("asan",   "ON", matrix.get("ENABLE_ASAN"))
defs["ENABLE_UBSAN"]       = in_job_name("ubsan",  "ON", matrix.get("ENABLE_UBSAN"))

# not encoded in job name
get(defs, "ENABLE_ASSERTIONS")
get(defs, "ENABLE_CCACHE")
get(defs, "ENABLE_LTO")
get(defs, "ENABLE_SCCACHE")

get(envs, "NODE_VERSION",       24)
get(envs, "BUILD_UNIT_TESTS",   "ON")
get(envs, "BUILD_BENCHMARKS",   "OFF")
get(envs, "RUN_UNIT_TESTS",     "ON")
get(envs, "RUN_CUCUMBER_TESTS", "ON")
get(envs, "RUN_NODE_TESTS",     defs.get("BUILD_NODE_PACKAGE"))
get(envs, "RUN_BENCHMARKS",     "OFF")

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

envs["COMPILER_ID"] = compiler
envs["COMPILER_VERSION"] = version

ver = "-" + version if version else ""
if compiler == "clang":
    envs["CC"] = f"clang{ver}"
    envs["CXX"] = f"clang++{ver}"
    if defs["ENABLE_TIDY"] == "ON":
        envs["CLANG_TIDY"] = f"clang-tidy{ver}"
    if defs["ENABLE_COVERAGE"] == "ON":
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
# fmt: on

defs["CMAKE_C_COMPILER"] = envs["CC"]
defs["CMAKE_CXX_COMPILER"] = envs["CXX"]
defs["CMAKE_CXX_CLANG_TIDY"] = envs["CLANG_TIDY"]
defs["CMAKE_C_FLAGS"] = envs["CFLAGS"]
defs["CMAKE_CXX_FLAGS"] = envs["CXXFLAGS"]

# store definitions for cmake -D...
definitions = []
for key in sorted(defs):
    if defs[key] is not None:
        definitions.append(f"-D{key}={defs[key]}")
definitions = " ".join(definitions)
print(f'MATRIX_CMAKE_DEFINITIONS="{definitions}"')

# values for storing into GITHUB_ENV
envs.update(defs)
for key in sorted(envs):
    if envs[key] is not None:
        # into .env file
        print(f"{key}={envs[key]}")
