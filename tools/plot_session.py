#!/usr/bin/env python3
"""Plot a recorded session: acceleration and rotation for both IMUs.

Usage:
    python3 tools/plot_session.py data/normal_walk_20261123-140000.csv

Markers (Enter presses during recording) are drawn as dashed vertical lines.
"""

import sys

import matplotlib.pyplot as plt
import numpy as np


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    d = np.genfromtxt(sys.argv[1], delimiter=",", names=True)
    t = (d["t_ms"] - d["t_ms"][0]) / 1000.0

    fig, axes = plt.subplots(4, 1, sharex=True, figsize=(12, 9))
    panels = [
        ("Handle acceleration (g)", "h_a"),
        ("Handle rotation (deg/s)", "h_g"),
        ("Tip acceleration (g)", "t_a"),
        ("Tip rotation (deg/s)", "t_g"),
    ]
    for ax, (title, prefix) in zip(axes, panels):
        for axis in "xyz":
            ax.plot(t, d[prefix + axis], label=axis, linewidth=0.8)
        ax.set_title(title, fontsize=10)
        ax.legend(loc="upper right", fontsize=8)
        ax.grid(alpha=0.3)

    for mt in t[d["mark"] > 0]:
        for ax in axes:
            ax.axvline(mt, color="k", linestyle="--", linewidth=0.8)

    missing = int((d["t_ok"] == 0).sum())
    axes[-1].set_xlabel("time (s)")
    fig.suptitle(f"{sys.argv[1]}  ({len(t)} rows, tip IMU missing in {missing})", fontsize=10)
    fig.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
