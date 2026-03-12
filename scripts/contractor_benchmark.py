import argparse
import datetime
import glob
import os
import pathlib
import re
import subprocess
import shutil
import tempfile

import pandas as pd
import tabulate


def run(args):
    for logfile in args.logfiles:
        pathlib.Path(logfile).unlink(missing_ok=True)

    while len(args.logfiles) < len(args.binaries):
        args.logfiles.append(args.logfiles[-1])

    for i in range(0, args.runs):
        for binary, logfile in zip(args.binaries, args.logfiles):
            with open(logfile, "a") as log:

                def tee(text):
                    if text != ".\n":
                        print(text, end="")
                        log.write(text)
                        log.flush()

                with tempfile.TemporaryDirectory(prefix="osrm-benchmark-") as tmp_dir:
                    for file in glob.glob(f"{args.dataset}.*"):
                        shutil.copy(file, tmp_dir)

                    tee(f"### {i} {binary} => {logfile}\n")
                    proc = subprocess.Popen(
                        [binary, os.path.join(tmp_dir, os.path.basename(args.dataset))],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT,
                        text=True,
                    )
                    while proc.poll() is None:
                        text = proc.stdout.readline()
                        tee(text)
    report(args)


def report(args):
    rows = list()
    index = list()
    for logfile in args.logfiles:
        with open(logfile) as log:
            for line in log:
                if m := re.search(r"^### (\d+) (.*) =>", line):
                    index.append((int(m.group(1)), m.group(2)))
                    rows.append({})
                if m := re.search(r"Contracted graph has (\d+) edges", line):
                    rows[-1]["edges"] = int(m.group(1))
                if m := re.search(r"Contraction took ([.\d]+) sec", line):
                    rows[-1]["time"] = float(m.group(1))
                if m := re.search(r"RAM: peak bytes used: (\d+)", line):
                    rows[-1]["mem"] = int(m.group(1))

    df = pd.DataFrame(
        rows, index=pd.MultiIndex.from_tuples(index, names=("run", "branch"))
    )
    df.mem /= 1024 * 1024

    print(f"## RAW data - {datetime.datetime.now().isoformat()}\n```")
    print(df)
    print("```")

    grouped = df.groupby("branch")
    agg = grouped.aggregate(["median", "std"])
    agg = agg.drop(columns=[("edges", "std"), ("mem", "std")])

    # agg = agg.reset_index(names="branch")

    agg = agg.assign(
        norm=pd.Series(agg["time", "median"] / agg["time", "median"].iloc[0]).values
    )

    print("\n## Summary\n")
    headers = ("branch", "edges", "time (s)", "std", "mem (MB)", "norm")
    floatfmt = ("", ".0f", ".2f", ".2f", ".2f", ".3f")
    print(
        tabulate.tabulate(
            agg, headers, tablefmt="github", floatfmt=floatfmt, showindex=True
        )
    )


def build_parser():
    parser = argparse.ArgumentParser(
        description="Benchmark and compare contractor binaries"
    )
    subparsers = parser.add_subparsers(help="subcommand help")

    parser_run = subparsers.add_parser("run", help="run the benchmark")
    parser_run.set_defaults(func=run)
    parser_run.add_argument(
        "--runs",
        help="How many times to run the comparison",
        type=int,
        default=10,
        metavar="NUM",
    )
    parser_run.add_argument("--dataset", help="path/to/dataset.osrm", metavar="PATH")
    parser_run.add_argument(
        "--logfiles",
        nargs="+",
        help="where to write the test logs",
        metavar="PATH",
    )
    parser_run.add_argument(
        "--binaries",
        nargs="+",
        help="The contractor binaries to compare",
        metavar="CONTRACTOR",
    )

    parser_report = subparsers.add_parser(
        "report", help="analyze the benchmark logfile"
    )
    parser_report.set_defaults(func=report)
    parser_report.add_argument(
        "--logfiles",
        nargs="+",
        help="the log file to process",
        metavar="PATH",
    )

    return parser


def main():
    args = build_parser().parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
