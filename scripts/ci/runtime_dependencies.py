"""
Copy the .so dependencies of a target.

Incredible but true: Cmake does not offer this very elementary function: to list the
dynamic libraries (.so) a target needs to run.

This tools does that.

Example: python scripts/ci/runtime_dependencies.py --grep "boost|tbb|osrm" build/Release/nodejs/node_osrm.node
"""

import argparse
import glob
import os
import platform
import re
import subprocess
import sys

args = argparse.Namespace()


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        "filenames",
        nargs="+",
        type=str,
        metavar="FILENAME",
    )
    parser.add_argument(
        "--grep",
        type=str,
        help="regular expression the libraries must match",
    )

    parser.parse_args(namespace=args)

    if args.grep:
        args.grep = re.compile(args.grep)

    if platform.system() == "Linux":
        # ldd filename
        tool = "ldd"
        # <TAB>libboost_date_time.so.1.83.0 => /path/to/libboost_date_time.so.1.83.0 (0x00007fcf3276f000)
        regex = re.compile(r" => (.*) \(0x")
        # regex = re.compile(r"\s+(.*) => (.*) \(0x")
        path = os.environ.get("LD_LIBRARY_PATH", "").split(":")
    if platform.system() == "Darwin":
        # otool -L libfoo.dylib
        tool = "otool -L"
        # @rpath/node_osrm.node (compatibility version 0.0.0, current version 0.0.0)
        # @rpath/libosrm.dylib (compatibility version 0.0.0, current version 0.0.0)
        # @rpath/libosrm_utils.dylib (compatibility version 0.0.0, current version 0.0.0)
        # @rpath/libboost_iostreams.dylib (compatibility version 0.0.0, current version 0.0.0)
        regex = re.compile(r"@rpath/(.*dylib)\s")
        path = os.environ.get("DYLD_LIBRARY_PATH", "").split(":")
    if platform.system() == "Windows":
        # https://learn.microsoft.com/en-us/cpp/build/reference/dependents?view=msvc-170
        # https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-Readme.md
        # Image has the following dependencies:
        #   KERNEL32.dll
        #   SHLWAPI.dll
        #   boost_iostreams.dll
        #   MSVCP140.dll
        tool = glob.glob(
            r"C:\\Program Files\\Microsoft Visual Studio\\**\\DUMPBIN.EXE",
            recursive=True,
        )
        if len(tool) > 0:
            tool = f'"{tool[0]}" /DEPENDENTS'
        else:
            print("Cannot find dumpbin.exe")
            exit(-1)
        regex = re.compile(r"^\s+(.*dll)$")
        path = os.environ.get("PATH", "").split(":")

    def find_lib(path: list[str], lib: str) -> str:
        """Find the library in PATH"""
        if "/" in lib:
            return lib
        for p in path:
            p = os.path.join(p, lib)
            if os.access(p, os.R_OK):
                return p
        return lib

    libs = set()
    for filename in args.filenames:
        stdout = subprocess.check_output(
            f"{tool} {filename}", shell=True, encoding="utf-8"
        )
        for line in stdout.splitlines():
            m = regex.search(line)
            if m:
                dir = os.path.dirname(filename)
                # be careful *not* to resolve symlinks! eg. libtbbmalloc.so.2 => libtbbmalloc.so.2.17
                libs.add(os.path.normpath(find_lib([dir] + path, m.group(1))))

    # always use \n even on Windows!
    sys.stdout.reconfigure(newline="\n")

    for lib in sorted(libs):
        if args.grep:
            if args.grep.search(lib):
                print(lib)
        else:
            print(lib)


if __name__ == "__main__":
    main()
