# VideoPlayer firmware

Looping MJPEG player for the Waveshare **ESP32-S3-Touch-AMOLED-1.75** (466×466 CO5300 AMOLED).

## Flash from the command line

**Requirements:** USB-C data cable, board plugged in.

**One-time setup:**

```bash
brew install arduino-cli
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install JPEGDEC
arduino-cli lib install "GFX Library for Arduino"
```

**Flash** (from this folder):

```bash
./flash.sh
```

If the port is not detected, hold **BOOT**, plug in USB, release **BOOT**, then run again.

Press **RESET** after upload. Insert the SD card (`video.avi` on the root) and connect the speaker.

---

## Hardware Required

| Part | Notes |
|------|-------|
| ESP32-S3-Touch-AMOLED-1.75 | Waveshare 466×466 AMOLED module |
| TF / micro-SD card | FAT32, any capacity ≤32 GB |
| Speaker | Plug into the MX1.25 speaker connector |
| USB-C cable | Data-capable (for flashing) |

Battery connects to the onboard MX1.25 lithium header (AXP2101 charging) — no external TP4057 needed.

---

## 1 – Prepare Your Video

```bash
./scripts/convert_for_sd.sh YOUR_INPUT.mp4
```

Or manually:

```bash
ffmpeg -i YOUR_INPUT.mp4 \
  -vf "scale=466:466:force_original_aspect_ratio=decrease,pad=466:466:(ow-iw)/2:(oh-ih)/2,setsar=1" \
  -r 15 \
  -vcodec mjpeg -q:v 8 \
  -af "highpass=f=150,alimiter=limit=-3dB" \
  -acodec pcm_s16le -ar 22050 -ac 1 \
  video.avi
```

Copy `video.avi` to the **root** of the TF card.

---

## Board settings

| Setting | Value |
|---------|-------|
| Board | `ESP32S3 Dev Module` |
| USB CDC On Boot | `Enabled` |
| Flash Size | `16MB` |
| Partition Scheme | `16M Flash (3MB APP/9.9MB FATFS)` |
| PSRAM | `OPI PSRAM` |
| CPU Frequency | `240MHz` |

---

## Files

```
firmware/
├── firmware.ino             Main sketch
├── AVI_Player.h/.cpp        MJPEG AVI parser + I2S → PCM5102A
├── Display_CO5300.h/.cpp    CO5300 QSPI AMOLED (Arduino_GFX)
├── SD_Card.h/.cpp           TF card (SPI: CLK2/CMD1/D03/CS41)
├── EC11_Volume.h/.cpp       Dial volume + standby
├── TCA9554PWR.h/.cpp        GPIO expander
├── I2C_Driver.h/.cpp        I2C (SDA15 / SCL14)
├── PWR_Key.h/.cpp           Power helpers (AXP2101 button)
└── BAT_Driver.h/.cpp        Battery via AXP2101
```

## Notes

- Audio uses an external **PCM5102A** on GPIO 18/17/16 (BCK/DIN/LCK); tie DAC SCK to GND.
- Volume and standby are controlled by the EC11 dial (TX/RX + EX0).
