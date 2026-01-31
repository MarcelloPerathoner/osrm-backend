import argparse
from collections import defaultdict
import csv
import gzip
import os
import random
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
        "--samples", type=int, help="Number of samples to take (1000)", default=1000
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
        "0.05 q (ms)",
        "Median (ms)",
        "0.95 q (ms)",
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

    random.seed(42)
    samples = np.ndarray(args.samples)

    runner = BenchmarkRunner(args.gps_traces)
    runner.run(samples, args.method, args.host)

    ms = samples * 1000.0

    values = [
        np.quantile(ms, 0.05),
        np.median(ms),
        np.quantile(ms, 0.95),
    ]
    for h, v in zip(headers, values):
        print(f"{h + ':':21} {v:.2f}")

    # running on github ci
    if "GITHUB_STEP_SUMMARY" in os.environ:
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as summary:
            summary.write(f"| {args.method:7}")
            for v in values:
                summary.write(f" | {v:.2f}")
            summary.write(" |\n")


if __name__ == "__main__":
    main()
