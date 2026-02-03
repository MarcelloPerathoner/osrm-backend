"""
Copy the .so dependencies of a target.

Incredible but true: Cmake does not offer this very elementary function: to list the
dynamic libraries (.so) a target needs to run.

This tools does that.

Example: python scripts/ci/runtime_dependencies.py --grep "boost|tbb|osrm" lib/binding_napi_v8/node_osrm.node
"""

import argparse
import re
import subprocess
import shutil

args = argparse.Namespace()


def process(path: str):
    if args.target:
        try:
            shutil.copy(path, args.target)
        except shutil.SameFileError:
            pass
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

    libs = {}

    # ldd
    # <TAB>libboost_date_time.so.1.83.0 => /path/to/libboost_date_time.so.1.83.0 (0x00007fcf3276f000)
    regex = re.compile(r"^\s*(.*) => (.*) \(0x")

    for filename in args.filenames:

        # macOS: otool -L
        # Windows: dumpbin /DEPENDENTS MathClient.exe
        with subprocess.Popen(
            ["ldd", filename], stdout=subprocess.PIPE, encoding="utf-8"
        ) as proc:
            for line in proc.stdout.readlines():
                m = regex.match(line)
                if m:
                    libs[m.group(1)] = m.group(2)

    for lib, path in libs.items():
        if args.grep:
            m = args.grep.search(lib)
            if m:
                process(path)
        else:
            process(path)


if __name__ == "__main__":
    main()
