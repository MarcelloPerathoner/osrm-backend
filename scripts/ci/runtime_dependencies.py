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
    parser.add_argument(
        "--debug",
        action="store_true",
        help="print the tool output and exit",
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
        # /home/me/lib/libjli.dylib:
        # 	@rpath/libjli.dylib (compatibility version 1.0.0, current version 1.0.0)
        # 	/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation (compatibility version 300.0.0, current version 1856.105.0)
        # 	/usr/lib/libobjc.A.dylib (compatibility version 1.0.0, current version 228.0.0)
        regex = re.compile(r"^\s+(/.*dylib)\s")
        path = os.environ.get("DYLD_LIBRARY_PATH", "").split(":")
    if platform.system() == "Windows":
        # https://learn.microsoft.com/en-us/cpp/build/reference/dependents?view=msvc-170
        # https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-Readme.md
        tool = glob.glob(
            r"C:\\Program Files\\Microsoft Visual Studio\\**\\DUMPBIN.EXE",
            recursive=True,
        )
        if len(tool) > 0:
            tool = tool[0] + " /DEPENDENTS"
            regex = re.compile(r"^\s+(.*dll)$")
        else:
            print("Cannot find dumpbin.exe")
            exit(-1)
        path = os.environ.get("PATH", "").split(":")

    def find_lib(lib: str) -> str:
        """Find the library in PATH"""
        if "/" in lib:
            return lib
        for p in path:
            p = os.path.join(p, lib)
            if os.access(p, os.R_OK):
                return p
        return lib

    def gather(binaries):
        libs = []
        for bin in list(binaries):
            stdout = subprocess.check_output(
                f"{tool} {bin}", shell=True, encoding="utf-8"
            )
            if args.debug:
                print(stdout)
                exit(0)
            for line in stdout.splitlines():
                m = regex.search(line)
                if m:
                    libs.append(find_lib(m.group(1)))
        return libs

    for lib in gather(args.filenames):
        # be careful *not* to resolve symlinks! eg. libtbbmalloc.so.2 => libtbbmalloc.so.2.17
        lib = os.path.normpath(lib)
        if args.grep:
            m = args.grep.search(lib)
            if m:
                print(lib)
        else:
            print(lib)


if __name__ == "__main__":
    main()
