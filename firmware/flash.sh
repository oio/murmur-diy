#!/usr/bin/env bash
# Flash VideoPlayer to Waveshare ESP32-S3-Touch-AMOLED-1.75
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/firmware"
FQBN="esp32:esp32:esp32s3"
BOARD_OPTS="CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,UploadSpeed=921600,CPUFreq=240"

detect_port() {
  arduino-cli board list 2>/dev/null \
    | awk '/esp32:esp32/ && /usbmodem|usbserial|wchusbserial|SLAB_USB/ { print $1; exit }'
}

if [[ -z "${PORT:-}" ]]; then
  PORT="$(detect_port)"
  if [[ -z "$PORT" ]]; then
    echo "No ESP32 serial port found."
    echo "Plug in the board via USB-C (hold BOOT while connecting if the port does not appear)."
    echo
    arduino-cli board list
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
