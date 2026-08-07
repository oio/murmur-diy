#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: ./convert_for_sd.sh <input_file.mp4> [output_file.avi]"
    exit 1
fi

INPUT_FILE="$1"
# If output file is not provided, use the input filename with .avi extension
OUTPUT_FILE="${2:-${INPUT_FILE%.*}.avi}"

echo "Converting $INPUT_FILE to $OUTPUT_FILE..."

ffmpeg -i "$INPUT_FILE" \
  -vf "scale=466:466:force_original_aspect_ratio=decrease,pad=466:466:(ow-iw)/2:(oh-ih)/2,setsar=1" \
  -r 15 \
  -vcodec mjpeg -q:v 8 \
  -af "highpass=f=150,alimiter=limit=-3dB" \
  -acodec pcm_s16le -ar 22050 -ac 1 \
  "$OUTPUT_FILE"

echo "Done! You can now copy $OUTPUT_FILE to your SD card."