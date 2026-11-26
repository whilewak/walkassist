#!/usr/bin/env python3
"""Plot a recorded session: acceleration and rotation for both IMUs, plus grip.

Usage:
    python3 tools/plot_session.py data/normal_walk_20261123-140000.csv

Markers (Enter presses during recording) are drawn as dashed vertical lines.
Gestures are drawn as dots on the grip panel (2 = double tap, 3 = triple tap),
with the grip pressure line when the recording has a `grip` column.
Older recordings without the touch / grip columns still plot.
"""

import sys

import matplotlib.pyplot as plt
import numpy as np


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    d = np.genfromtxt(sys.argv[1], delimiter=",", names=True)
    names = d.dtype.names
    t = (d["t_ms"] - d["t_ms"][0]) / 1000.0
    has_touch = "touch" in names

    panels = [
        ("Handle acceleration (g)", "h_a"),
        ("Handle rotation (deg/s)", "h_g"),
        ("Tip acceleration (g)", "t_a"),
        ("Tip rotation (deg/s)", "t_g"),
    ]
    n = len(panels) + (1 if has_touch else 0)
    ratios = [3] * len(panels) + ([2] if has_touch else [])
    fig, axes = plt.subplots(n, 1, sharex=True, figsize=(12, 10),
                             gridspec_kw={"height_ratios": ratios})

    for ax, (title, prefix) in zip(axes, panels):
        for axis in "xyz":
            ax.plot(t, d[prefix + axis], label=axis, linewidth=0.8)
        ax.set_title(title, fontsize=10)
        ax.legend(loc="upper right", fontsize=8)
        ax.grid(alpha=0.3)

    if has_touch:
        ax = axes[-1]
        has_grip = "grip" in names          # FSR pressure (0..1023), newer recordings
        top = 1023 if has_grip else 1
        ax.fill_between(t, d["touch"] * top, step="post", alpha=0.25, label="pressed")
        if has_grip:
            ax.plot(t, d["grip"], color="C3", linewidth=0.9, label="grip pressure")
        if "gesture" in names:
            g = d["gesture"] > 0
            ax.scatter(t[g], np.full(g.sum(), top / 2), color="k", zorder=3, label="gesture")
            for tx, code in zip(t[g], d["gesture"][g]):
                ax.annotate("double" if code == 2 else "triple", (tx, top / 2),
                            textcoords="offset points", xytext=(4, 4), fontsize=8)
        ax.set_ylim(-0.1 * top, 1.1 * top)
        if not has_grip:
            ax.set_yticks([0, 1])
        ax.set_title("Grip (FSR pressure 0-1023)" if has_grip else "Grip", fontsize=10)
        ax.legend(loc="upper right", fontsize=8)

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
