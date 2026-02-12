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
# fmt: on

# guess the compiler from the job name
# may be overwritten explicitly
m = re.search(r"(clang|gcc)-(\d+)", job_name)
if m:
    compiler = m.group(1)
    version = m.group(2)
    if compiler == "clang":
        values["CC"] = f"clang-{version}"
        values["CXX"] = f"clang++-{version}"
        values["CLANG_TIDY"] = f"clang-tidy-{version}"
    if compiler == "gcc":
        values["CC"] = f"gcc-{version}"
        values["CXX"] = f"g++-{version}"
else:
    # default compiler is clang
    values["CC"] = "clang"
    values["CXX"] = "clang++"
    values["CLANG_TIDY"] = "clang-tidy"

if values["ENABLE_TIDY"] == "OFF":
    values.pop("CLANG_TIDY", None)

# fmt: off
get("CC")
get("CFLAGS")
get("CXX")
get("CXXFLAGS")
get("CLANG_TIDY")
get("USE_CCACHE",         "ccache")

get("NODE_VERSION",       24)
get("ENABLE_ASSERTIONS",  "OFF")
get("ENABLE_COVERAGE",    values["ENABLE_COVERAGE"])
get("ENABLE_ASAN",        values["ENABLE_ASAN"])
get("ENABLE_UBSAN",       values["ENABLE_UBSAN"])
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

# { "name": "conan-clang-42-debug-shared-node-tidy-asan-ubsan-cov", "RUN_NODE_TESTS": "OFF" }
