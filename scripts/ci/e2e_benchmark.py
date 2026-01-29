import argparse
from collections import defaultdict
import csv
import gzip
import os
import random
from statistics import NormalDist
import time

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
        self, benchmark_name, host, num_requests, warmup_requests=50
    ) -> list[float]:
        times: list[float] = []

        for i in range(warmup_requests + num_requests):
            url = self.make_url(host, benchmark_name)
            start_time = time.time()
            response = requests.get(url)
            end_time = time.time()
            if response.status_code != 200:
                code = response.json()["code"]
                if code not in ["NoSegment", "NoMatch", "NoRoute", "NoTrips"]:
                    raise Exception(f"Error: {response.status_code} {response.text}")
            if i >= warmup_requests:
                times.append(end_time - start_time)

        return times

    def make_url(self, host, benchmark_name):
        def toString(coords) -> str:
            return ";".join([f"{coord[1]:.6f},{coord[0]:.6f}" for coord in coords])

        if benchmark_name == "route":
            coords = random.sample(self.coordinates, 2)
            return (
                f"{host}/route/v1/driving/{toString(coords)}?overview=full&steps=true"
            )
        elif benchmark_name == "nearest":
            coords = random.sample(self.coordinates, 1)
            return f"{host}/nearest/v1/driving/{toString(coords)}"
        elif benchmark_name == "table":
            num_coords = random.randint(3, 12)
            coords = random.sample(self.coordinates, num_coords)
            return f"{host}/table/v1/driving/{toString(coords)}"
        elif benchmark_name == "trip":
            num_coords = random.randint(2, 10)
            coords = random.sample(self.coordinates, num_coords)
            return f"{host}/trip/v1/driving/{toString(coords)}?steps=true"
        elif benchmark_name == "match":
            num_coords = random.randint(50, 100)
            track_id = random.choice(self.track_ids)
            coords = self.tracks[track_id][:num_coords]
            radiuses_str = ";".join(
                [f"{random.randint(20, 100)}" for _ in range(len(coords))]
            )
            return f"{host}/match/v1/driving/{toString(coords)}?steps=true&radiuses={radiuses_str}"
        else:
            raise Exception(f"Unknown benchmark: {benchmark_name}")


def conf(data, confidence=0.95):
    """Calculate the confidence interval for the given confidence.

    Example: If `confidence` is given as 0.95, then we expect that 95% of the values
    will fall into the calculated interval."""
    dist = NormalDist.from_samples(data)
    z = NormalDist().inv_cdf((1 + confidence) / 2.0)
    h = dist.stdev * z / ((len(data) - 1) ** 0.5)
    return dist.mean, h


def main():
    parser = argparse.ArgumentParser(description="Run GPS benchmark tests.")
    parser.add_argument("--host", type=str, required=True, help="Host URL")
    parser.add_argument(
        "--method",
        type=str,
        required=True,
        choices=["route", "table", "match", "nearest", "trip"],
        help="Benchmark method",
    )
    parser.add_argument(
        "--iterations",
        type=int,
        required=True,
        help="Number of iterations to run",
    )
    parser.add_argument(
        "--gps_traces",
        type=str,
        required=True,
        help="Path to the GPS traces file",
    )

    args = parser.parse_args()

    np.random.seed(42)
    random.seed(42)

    runner = BenchmarkRunner(args.gps_traces)

    times = np.asarray(runner.run(args.method, args.host, args.iterations))
    ms = times * 1000.0

    try:
        print(f"Min time:        {np.min(ms):.2f}ms")
        print(f"Median time:     {np.median(ms):.2f}ms")
        print(f"Mean time:       {np.mean(ms):.2f}ms")
        print("95th percentile: {:.2f}ms ± {:.3f}ms".format(*conf(ms, 0.95)))
        print("99th percentile: {:.2f}ms ± {:.3f}ms".format(*conf(ms, 0.99)))
        print(f"Max time:        {np.max(ms):.2f}ms")
        print("Ops/s 95th:      {:.2f} ± {:.3f}".format(*conf(1 / times, 0.95)))

    except ValueError as exc:
        print(exc)
        pass


if __name__ == "__main__":
    main()
