#!/usr/bin/env bash
# Compile, upload and monitor sketches for the Nano 33 IoT with arduino-cli.
# Used by the VS Code tasks in .vscode/tasks.json; also works from a terminal:
#
#   tools/arduino.sh compile firmware/imu_logger
#   tools/arduino.sh upload  firmware/imu_logger      (compiles first)
#   tools/arduino.sh monitor
#   tools/arduino.sh boards
#
# The sketch can be the folder or any file inside it.
# The port is found automatically; set PORT=/dev/cu.usbmodemXXXX to override.

set -euo pipefail

FQBN="arduino:samd:nano_33_iot"
BAUD=115200
REPO="$(cd "$(dirname "$0")/.." && pwd)"

usage() {
  echo "usage: $0 compile|upload <sketch folder or file>" >&2
  echo "       $0 monitor|boards|port" >&2
  exit 1
}

command -v arduino-cli >/dev/null || { echo "arduino-cli not found: brew install arduino-cli"; exit 1; }

sketch_dir() {
  local target="${1:-}"
  [ -n "$target" ] || usage
  [ -d "$target" ] || target="$(dirname "$target")"
  local name
  name="$(basename "$target")"
  if [ ! -f "$target/$name.ino" ]; then
    echo "Not a sketch: $target (expected $target/$name.ino)." >&2
    echo "Open a file inside a sketch folder, e.g. firmware/imu_logger/imu_logger.ino." >&2
    exit 1
  fi
  echo "$target"
}

find_port() {
  if [ -n "${PORT:-}" ]; then echo "$PORT"; return; fi
  local port
  port="$(arduino-cli board list --format json | python3 -c '
import json, sys
data = json.load(sys.stdin)
ports = data.get("detected_ports", data) if isinstance(data, dict) else data
for p in ports:
    props = p.get("port", {}).get("properties", {})
    fqbns = [b.get("fqbn", "") for b in p.get("matching_boards", [])]
    if "arduino:samd:nano_33_iot" in fqbns or props.get("vid", "").lower() == "0x2341":
        print(p["port"]["address"])
        break
')"
  if [ -z "$port" ]; then
    echo "No Nano 33 IoT found. Plug it in (data cable, not charge-only)," >&2
    echo "close any Serial Monitor using it, or set PORT=/dev/cu.usbmodemXXXX." >&2
    exit 1
  fi
  echo "$port"
}

cmd="${1:-}"
case "$cmd" in
  compile)
    dir="$(sketch_dir "${2:-}")"
    arduino-cli compile --fqbn "$FQBN" --build-path "$REPO/.build/$(basename "$dir")" "$dir"
    ;;
  upload)
    dir="$(sketch_dir "${2:-}")"
    port="$(find_port)"
    echo "Uploading $(basename "$dir") to $port"
    arduino-cli compile --fqbn "$FQBN" --build-path "$REPO/.build/$(basename "$dir")" \
      --upload --port "$port" "$dir"
    ;;
  monitor)
    port="$(find_port)"
    echo "Serial monitor on $port at $BAUD baud. Ctrl+C to quit."
    arduino-cli monitor --port "$port" --config "baudrate=$BAUD"
    ;;
  boards)
    arduino-cli board list
    ;;
  port)
    find_port
    ;;
  *)
    usage
    ;;
esac
