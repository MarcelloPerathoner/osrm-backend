import argparse
from collections import defaultdict
import csv
import gzip
import os
import random
from statistics import NormalDist
import time
import sys

import numpy as np
import requests


class BenchmarkRunner:
    def __init__(self, gps_traces):
        self.coordinates = []
        self.tracks = defaultdict(list)

        gps_traces_file_path = os.path.expanduser(gps_traces)

        if gps_traces_file_path.endswith(".gz"):
            infile = gzip.open(gps_traces_file_path, "rt")
        else:
            infile = open(gps_traces_file_path, "rt")

        with infile as file:
            reader = csv.DictReader(file)
            for row in reader:
                coord = (float(row["Latitude"]), float(row["Longitude"]))
                self.coordinates.append(coord)
                self.tracks[row["TrackID"]].append(coord)
        self.track_ids = list(self.tracks.keys())

    def run(
        self, samples: np.ndarray, benchmark_name, host, warmup_requests=50
    ) -> list[float]:
        for i in range(-warmup_requests, samples.size):
            url = self.make_url(host, benchmark_name)
            start_time = time.time()
            response = requests.get(url)
            end_time = time.time()
            if response.status_code != 200:
                code = response.json()["code"]
                if code not in ["NoSegment", "NoMatch", "NoRoute", "NoTrips"]:
                    raise Exception(f"Error: {response.status_code} {response.text}")
            if i >= 0:
                samples.flat[i] = end_time - start_time

    def make_url(self, host, benchmark_name):
        def toString(coords) -> str:
            return ";".join([f"{coord[1]:.6f},{coord[0]:.6f}" for coord in coords])

        url = f"{host}/{benchmark_name}/v1/driving"

        if benchmark_name == "route":
            coords = random.sample(self.coordinates, 2)
            return f"{url}/{toString(coords)}?overview=full&steps=true"
        elif benchmark_name == "nearest":
            coords = random.sample(self.coordinates, 1)
            return f"{url}/{toString(coords)}"
        elif benchmark_name == "table":
            num_coords = random.randint(3, 12)
            coords = random.sample(self.coordinates, num_coords)
            return f"{url}/{toString(coords)}"
        elif benchmark_name == "trip":
            num_coords = random.randint(2, 10)
            coords = random.sample(self.coordinates, num_coords)
            return f"{url}/{toString(coords)}?steps=true"
        elif benchmark_name == "match":
            num_coords = random.randint(50, 100)
            track_id = random.choice(self.track_ids)
            coords = self.tracks[track_id][:num_coords]
            radiuses_str = ";".join(
                [f"{random.randint(10, 20)}" for _ in range(len(coords))]
            )
            return f"{url}/{toString(coords)}?steps=true&radiuses={radiuses_str}"
        else:
            raise Exception(f"Unknown benchmark: {benchmark_name}")


def conf(data, confidence=0.95):
    """Calculate the confidence interval for the given confidence.

    Example: If `confidence` is given as 0.95, then we expect that 95% of the values
    will fall into the calculated interval."""

    dist = NormalDist.from_samples(data)
    z = NormalDist().inv_cdf((1 + confidence) / 2.0)
    h = dist.stdev * z / ((len(data) - 1) ** 0.5)
    return f"{dist.mean:.2f} ± {h:.3f}"


def main():
    parser = argparse.ArgumentParser(description="Run GPS benchmark tests.")

    parser.add_argument(
        "--host",
        type=str,
        default="http://localhost:5000",
        help="Host URL (localhost:5000)",
    )
    parser.add_argument(
        "--method",
        type=str,
        choices=["route", "table", "match", "nearest", "trip"],
        default="route",
        help="Benchmark method",
    )
    parser.add_argument(
        "--requests", type=int, help="Number of requests per sample (50)", default=50
    )
    parser.add_argument(
        "--samples", type=int, help="Number of samples to take (100)", default=100
    )
    parser.add_argument(
        "--gps_traces",
        type=str,
        help="Path to the GPS traces file",
    )
    parser.add_argument(
        "--headers", help="Write table headers only", action="store_true"
    )

    args = parser.parse_args()

    headers = [
        "Ops/s",
        "Min time (ms)",
        "Median time (ms)",
        "Mean time (ms)",
        "95th percentile (ms)",
        "99th percentile (ms)",
        "Max time (ms)",
    ]

    if args.headers:
        if "GITHUB_STEP_SUMMARY" in os.environ:
            with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as summary:
                summary.write("| Method  | ")
                summary.write(" | ".join(headers))
                summary.write(" |\n| ------- | ")
                summary.write("| ".join([("-" * len(h) + ":") for h in headers]))
                summary.write("|\n")
        sys.exit()

    np.random.seed(42)
    random.seed(42)

    runner = BenchmarkRunner(args.gps_traces)

    samples = np.ndarray((args.samples, args.requests))
    runner.run(samples, args.method, args.host)

    ms = samples * 1000.0
    ops = 1.0 / samples

    values = [
        conf(np.mean(ops, 1)),
        conf(np.min(ms, 1)),
        conf(np.median(ms, 1)),
        conf(np.mean(ms, 1)),
        conf(np.percentile(ms, 95, 1)),
        conf(np.percentile(ms, 99, 1)),
        conf(np.max(ms, 1)),
    ]
    for h, v in zip(headers, values):
        print(f"{h + ":":21} {v}")

    # running on github ci
    if "GITHUB_STEP_SUMMARY" in os.environ:
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as summary:
            summary.write(f"| {args.method:7} | ")
            summary.write(" | ".join(values))
            summary.write(" |\n")


if __name__ == "__main__":
    main()
