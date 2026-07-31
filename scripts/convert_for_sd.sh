#!/usr/bin/env bash
# Convert MP4 video to AVI format suitable for the Waveshare
# ESP32-S3-Touch-AMOLED-1.75 SD card playback (466×466).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VIDEO="${1:-$ROOT/media/murmur_record_1784021022859.mp4}"
OUT="${2:-$ROOT/media/video.avi}"

if [[ ! -f "$VIDEO" ]]; then
  echo "Video not found: $VIDEO"
  exit 1
fi

echo "Converting ${VIDEO} -> ${OUT} for SD card playback (466x466)..."
ffmpeg -y -hide_banner -loglevel warning -i "$VIDEO" \
  -vf "scale=466:466:force_original_aspect_ratio=decrease,pad=466:466:(ow-iw)/2:(oh-ih)/2,setsar=1" \
  -r 15 \
  -vcodec mjpeg -q:v 8 \
  -acodec pcm_s16le -ar 22050 -ac 1 \
  "$OUT"

echo "Done! You can place $OUT on the root of your SD card."
