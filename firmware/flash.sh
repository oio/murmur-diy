#!/usr/bin/env bash
# Flash VideoPlayer to Waveshare ESP32-S3-Touch-AMOLED-1.75
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/firmware"
FQBN="esp32:esp32:esp32s3"
BOARD_OPTS="CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,UploadSpeed=921600,CPUFreq=240"

is_esp_upload_port() {
  local addr="$1"
  [[ "$addr" =~ (Bluetooth|debug-console|Incoming-Port) ]] && return 1
  [[ "$addr" =~ (usbmodem|usbserial|wchusbserial|SLAB_USB|ttyACM|ttyUSB|^COM[0-9]+$) ]]
}

detect_port() {
  local list heuristic=() addr

  list="$(arduino-cli board list --discovery-timeout 5s 2>/dev/null)" || return 1

  # Prefer a port where Arduino CLI already matched an ESP32 board (works for COM3 on Windows too).
  while IFS= read -r addr; do
    [[ -n "$addr" ]] || continue
    printf '%s\n' "$addr"
    return 0
  done < <(printf '%s\n' "$list" | awk 'NR > 1 && /esp32:esp32/ { print $1; exit }')

  while IFS= read -r addr; do
    [[ -n "$addr" ]] || continue
    if is_esp_upload_port "$addr"; then
      heuristic+=("$addr")
    fi
  done < <(printf '%s\n' "$list" | awk 'NR > 1 { print $1 }')

  if ((${#heuristic[@]} == 1)); then
    printf '%s\n' "${heuristic[0]}"
    return 0
  fi

  if ((${#heuristic[@]} > 1)); then
    echo "Multiple possible upload ports:" >&2
    printf '  %s\n' "${heuristic[@]}" >&2
    echo "Set PORT explicitly, e.g. PORT=${heuristic[0]} ./flash.sh" >&2
    return 1
  fi

  return 1
}

if [[ -z "${PORT:-}" ]]; then
  if ! PORT="$(detect_port)"; then
    echo "No ESP32 serial port found."
    echo "Plug in the board via USB-C (hold BOOT while connecting if the port does not appear)."
    echo "Or set PORT manually, e.g. PORT=COM3 ./flash.sh  or  PORT=/dev/cu.usbmodem101 ./flash.sh"
    echo
    arduino-cli board list --discovery-timeout 5s
    exit 1
  fi
fi

ensure_lib() {
  local name="$1"
  if ! arduino-cli lib list 2>/dev/null | grep -qi "$name"; then
    echo "Installing $name..."
    arduino-cli lib install "$name"
  fi
}
ensure_lib "GFX Library for Arduino"
ensure_lib "JPEGDEC"

cleanup() { rm -rf "$BUILD_DIR"; }
trap cleanup EXIT

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cp "$SCRIPT_DIR"/firmware.ino "$BUILD_DIR/"
cp "$SCRIPT_DIR"/*.{cpp,h} "$BUILD_DIR/" 2>/dev/null || true

echo "Compiling..."
arduino-cli compile --fqbn "$FQBN" --board-options "$BOARD_OPTS" "$BUILD_DIR"

echo "Uploading (port: $PORT)..."
arduino-cli upload --fqbn "$FQBN" --board-options "$BOARD_OPTS" -p "$PORT" "$BUILD_DIR"

echo "Done. Press RESET if the board does not start."
