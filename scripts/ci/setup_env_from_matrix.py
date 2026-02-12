import json
import os
import sys

matrix = json.load(sys.stdin)
values = {}


def in_name(needle, value, default):
    """Return value if the needle was found in the job name else default."""
    return value if needle in matrix.get("name") else default


def get(key, default=None):
    """Get a value from the job matrix with default."""
    values[key] = matrix.get(key, default)


# fmt: off
values["ENABLE_CONAN"]       = in_name("conan",  "ON",    "OFF")
values["BUILD_TYPE"]         = in_name("debug",  "Debug", "Release")
values["BUILD_SHARED_LIBS"]  = in_name("shared", "ON",    "OFF")
values["BUILD_NODE_PACKAGE"] = in_name("node",   "ON",    "OFF")

get("CCOMPILER")
get("CFLAGS")
get("CXXCOMPILER")
get("CXXFLAGS")
get("CLANG_TIDY")
get("USE_CCACHE",         "ccache")

get("NODE_VERSION",       24)
get("ENABLE_ASSERTIONS",  "OFF")
get("ENABLE_COVERAGE",    "OFF")
get("ENABLE_ASAN",        "OFF")
get("ENABLE_UBSAN",       "OFF")
get("ENABLE_LTO",         "ON")

get("BUILD_UNIT_TESTS",   "ON")
get("RUN_UNIT_TESTS",     "ON")
get("RUN_CUCUMBER_TESTS", "ON")
get("RUN_NODE_TESTS",     values["BUILD_NODE_PACKAGE"])
get("BUILD_BENCHMARKS",   "OFF")
get("RUN_BENCHMARKS",     "OFF")

# fmt: on

# store values for following job steps
for key in sorted(values):
    if values[key] is not None:
        print(f"{key}={values[key]}")

# { "name": "conan-debug-shared-node", "RUN_NODE_TESTS": "OFF" }
