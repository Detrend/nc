#!/usr/bin/env python3
"""Plot raycast profiling data: histogram of times and histogram of from/to distances."""

import argparse
import csv
import math

import matplotlib.pyplot as plt


def parse_point(text):
    x, y, z = text.strip("[]").split(":")
    return float(x), float(y), float(z)


def load_rows(filename):
    times = []
    distances = []
    with open(filename, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            times.append(float(row["time"]))  # already in ms
            fx, fy, fz = parse_point(row["from"])
            tx, ty, tz = parse_point(row["to"])
            distances.append(math.dist((fx, fy, fz), (tx, ty, tz)))
    return times, distances


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("filename", help="CSV file with raycast profiling data")
    parser.add_argument("--bins", type=int, default=50, help="Number of histogram bins (default: 50)")
    args = parser.parse_args()

    times, distances = load_rows(args.filename)

    fig, (ax_time, ax_dist) = plt.subplots(1, 2, figsize=(12, 5))

    ax_time.hist(times, bins=args.bins, color="tab:blue", edgecolor="black")
    ax_time.set_xlabel("Time (ms)")
    ax_time.set_ylabel("Count")
    ax_time.set_title("Raycast Times")

    mean_time = sum(times) / len(times)
    min_time = min(times)
    max_time = max(times)
    per_second = 1000.0 / mean_time
    per_frame = 5.0 / mean_time  # 5 ms frame budget == 200 FPS
    stats_text = (
        f"Mean: {mean_time:.4f} ms\n"
        f"Min: {min_time:.4f} ms\n"
        f"Max: {max_time:.4f} ms\n"
        f"Raycasts/s: {per_second:.1f}\n"
        f"Raycasts/frame (5ms/200FPS): {per_frame:.1f}"
    )
    ax_time.text(
        0.98, 0.95, stats_text,
        transform=ax_time.transAxes,
        ha="right", va="top",
        bbox=dict(boxstyle="round", facecolor="white", alpha=0.8),
    )

    ax_dist.hist(distances, bins=args.bins, color="tab:orange", edgecolor="black")
    ax_dist.set_xlabel("Distance (from -> to)")
    ax_dist.set_ylabel("Count")
    ax_dist.set_title("Raycast Distances")

    fig.suptitle(args.filename)
    fig.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
