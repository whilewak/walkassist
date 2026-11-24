#!/usr/bin/env python3
"""Record the cane's IMU stream to a labeled CSV file.

Usage:
    python3 tools/log_serial.py --label normal_walk
    python3 tools/log_serial.py --label dropped --port /dev/cu.usbmodem14101

While recording, press Enter to drop a marker (e.g. at the moment you drop the
cane). Markers go in the last column, "mark": 0 normally, 1 at the first Enter,
2 at the second, and so on. Press Ctrl+C to stop.

Files are saved to data/<label>_<YYYYmmdd-HHMMSS>.csv.
"""

import argparse
import sys
import threading
import time
from datetime import datetime
from pathlib import Path

import serial
from serial.tools import list_ports

ARDUINO_VID = 0x2341
HEADER_START = "t_ms,"


def find_port():
    ports = list(list_ports.comports())
    for p in ports:
        if p.vid == ARDUINO_VID:
            return p.device
    names = ", ".join(p.device for p in ports) or "none"
    sys.exit(f"No Arduino found. Ports seen: {names}. Pass --port.")


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--label", required=True, help="what this session is, e.g. normal_walk")
    parser.add_argument("--port", help="serial port (auto-detected if omitted)")
    parser.add_argument("--out", default="data", help="output folder (default: data)")
    args = parser.parse_args()

    port = args.port or find_port()
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)
    path = out_dir / f"{args.label}_{datetime.now():%Y%m%d-%H%M%S}.csv"

    mark_count = 0
    pending_mark = 0
    lock = threading.Lock()

    def watch_enter():
        nonlocal mark_count, pending_mark
        for _ in sys.stdin:
            with lock:
                mark_count += 1
                pending_mark = mark_count
            print(f"  marker {mark_count}")

    with serial.Serial(port, 115200, timeout=1) as ser, open(path, "w") as f:
        time.sleep(0.5)
        ser.reset_input_buffer()
        ser.write(b"v")  # make sure the board is in CSV mode and re-sends the header

        print(f"Recording from {port} to {path}")
        print("Press Enter to add a marker, Ctrl+C to stop.\n")
        threading.Thread(target=watch_enter, daemon=True).start()

        have_header = False
        n_commas = 0
        col = {}
        rows = 0
        tip_missing = 0
        last_report = time.time()
        rows_at_report = 0

        try:
            while True:
                line = ser.readline().decode("ascii", errors="replace").strip()
                if not line:
                    continue
                if line.startswith("#"):
                    print(line)  # status message from the board
                    continue
                if line.startswith(HEADER_START):
                    if not have_header:
                        f.write(line + ",mark\n")
                        have_header = True
                        n_commas = line.count(",")
                        col = {name: i for i, name in enumerate(line.split(","))}
                    continue
                if not have_header or line.count(",") != n_commas:
                    continue  # partial line or plotter output

                with lock:
                    mark, pending_mark = pending_mark, 0
                f.write(f"{line},{mark}\n")
                rows += 1
                fields = line.split(",")
                if "t_ok" in col and fields[col["t_ok"]] == "0":
                    tip_missing += 1

                now = time.time()
                if now - last_report >= 2:
                    rate = (rows - rows_at_report) / (now - last_report)
                    grip = ""
                    if "touch" in col:
                        grip = ", grip touched" if fields[col["touch"]] == "1" else ", grip free"
                    print(f"  {rows} rows, {rate:.0f} rows/s, tip IMU missing in {tip_missing}{grip}")
                    last_report, rows_at_report = now, rows
        except KeyboardInterrupt:
            pass

    print(f"\nSaved {rows} rows to {path}")


if __name__ == "__main__":
    main()
