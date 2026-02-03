"""
Copy the .so dependencies of a target.

Incredible but true: Cmake does not offer this very elementary function: to list the
dynamic libraries (.so) a target needs to run.

This tools does that.

Example: python scripts/ci/runtime_dependencies.py --grep "boost|tbb|osrm" lib/binding_napi_v8/node_osrm.node
"""

import argparse
import glob
import platform
import re
import subprocess
import shutil

args = argparse.Namespace()


def process(path: str):
    if args.target:
        try:
            print(f"Copying {path}")
            shutil.copy(path, args.target)
        except shutil.SameFileError:
            pass
    else:
        print(path)


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
        "-t",
        "--target",
        type=str,
        help="the target directory",
    )

    parser.parse_args(namespace=args)

    if args.grep:
        args.grep = re.compile(args.grep)

    if platform.system() == "Linux":
        # ldd filename
        tool = ["ldd"]
        # <TAB>libboost_date_time.so.1.83.0 => /path/to/libboost_date_time.so.1.83.0 (0x00007fcf3276f000)
        regex = re.compile(r" => (.*) \(0x")
    if platform.system() == "Darwin":
        # otool -L libfoo.dylib
        tool = ["otool", "-L"]
        # /home/me/lib/libjli.dylib:
        # 	@rpath/libjli.dylib (compatibility version 1.0.0, current version 1.0.0)
        # 	/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation (compatibility version 300.0.0, current version 1856.105.0)
        # 	/usr/lib/libobjc.A.dylib (compatibility version 1.0.0, current version 228.0.0)
        regex = re.compile(r"^\s+(/.*dylib)\s")
    if platform.system() == "Windows":
        # https://learn.microsoft.com/en-us/cpp/build/reference/dependents?view=msvc-170
        # https://github.com/actions/runner-images/blob/main/images/windows/Windows2025-Readme.md
        tool = glob.glob(
            r"C:\\Program Files\\Microsoft Visual Studio\\**\\DUMPBIN.EXE",
            recursive=True,
        )
        if len(tool) > 0:
            print(f"Found: {tool[0]}")
            tool = [tool[0], "/DEPENDENTS"]
            regex = re.compile(r"^\s+(.*dll)$")
        # FIXME: find the path of the dlls using PATH

    libs = set()

    for filename in args.filenames:
        tool += [filename]
        print(f"Running: {tool}")
        with subprocess.Popen(tool, stdout=subprocess.PIPE, encoding="utf-8") as proc:
            for line in proc.stdout.readlines():
                print(line)
                m = regex.search(line)
                if m:
                    libs.add(m.group(1))

    for path in libs:
        if args.grep:
            m = args.grep.search(path)
            if m:
                process(path)
        else:
            process(path)


if __name__ == "__main__":
    main()
