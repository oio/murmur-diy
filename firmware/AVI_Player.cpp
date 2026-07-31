/**
 * AVI_Player.cpp
 *
 * Plays an AVI file (MJPEG + PCM16 audio) stored on the TF/SD card.
 * Video frames are decoded with JPEGDEC and pushed to the CO5300 466×466 AMOLED.
 * Audio is streamed via I2S to the onboard ES8311 codec.
 *
 * Expected AVI format (create with the ffmpeg command in README.md):
 *   - Video: MJPEG, 466×466, 15 fps, Q≈6–9
 *   - Audio: PCM signed 16-bit, 22050 Hz, mono or stereo
 *
 * Libraries required (install via Arduino Library Manager):
 *   - JPEGDEC  by Larry Bank
 *   - GFX Library for Arduino (CO5300)
 */

#include "AVI_Player.h"
#include <JPEGDEC.h>
#include "driver/i2s_std.h"
#include "Display_CO5300.h"
#include "SD_Card.h"
#include <SD.h>
#include "EC11_Volume.h"
#include "Theme_Manager.h"

// External PCM5102A on header pins (SCK left unconnected — DAC PLL from BCK/LRCK)
#define I2S_BCK_PIN   18
#define I2S_DOUT_PIN  17
#define I2S_LRCK_PIN  16

// ─── FOURCC helpers ───────────────────────────────────────────────────────────

#define MK4CC(a,b,c,d) ((uint32_t)((uint8_t)(a) | ((uint8_t)(b)<<8) | ((uint8_t)(c)<<16) | ((uint8_t)(d)<<24)))

static const uint32_t CC_RIFF = MK4CC('R','I','F','F');
static const uint32_t CC_AVI  = MK4CC('A','V','I',' ');
static const uint32_t CC_LIST = MK4CC('L','I','S','T');
static const uint32_t CC_hdrl = MK4CC('h','d','r','l');
static const uint32_t CC_movi = MK4CC('m','o','v','i');
static const uint32_t CC_avih = MK4CC('a','v','i','h');
static const uint32_t CC_strh = MK4CC('s','t','r','h');
static const uint32_t CC_strf = MK4CC('s','t','r','f');
static const uint32_t CC_vids = MK4CC('v','i','d','s');
static const uint32_t CC_auds = MK4CC('a','u','d','s');
static const uint32_t CC_00dc = MK4CC('0','0','d','c');
static const uint32_t CC_01dc = MK4CC('0','1','d','c');
static const uint32_t CC_01wb = MK4CC('0','1','w','b');
static const uint32_t CC_00wb = MK4CC('0','0','w','b');
static const uint32_t CC_idx1 = MK4CC('i','d','x','1');
static const uint32_t CC_JUNK = MK4CC('J','U','N','K');
static const uint32_t CC_IDIT = MK4CC('I','D','I','T');
static const uint32_t CC_rec  = MK4CC('r','e','c',' ');

// ─── AVI on-disk structures ───────────────────────────────────────────────────

#pragma pack(push, 1)
struct MainAVIHeader {
  uint32_t dwMicroSecPerFrame;
  uint32_t dwMaxBytesPerSec;
  uint32_t dwPaddingGranularity;
  uint32_t dwFlags;
  uint32_t dwTotalFrames;
  uint32_t dwInitialFrames;
  uint32_t dwStreams;
  uint32_t dwSuggestedBufferSize;
  uint32_t dwWidth;
  uint32_t dwHeight;
  uint32_t dwReserved[4];
};

struct AVIStreamHeader {
  char     fccType[4];
  char     fccHandler[4];
  uint32_t dwFlags;
  uint16_t wPriority;
  uint16_t wLanguage;
  uint32_t dwInitialFrames;
  uint32_t dwScale;
  uint32_t dwRate;
  uint32_t dwStart;
  uint32_t dwLength;
  uint32_t dwSuggestedBufferSize;
  uint32_t dwQuality;
  uint32_t dwSampleSize;
  struct { int16_t left, top, right, bottom; } rcFrame;
};

struct WaveFormatEx {
  uint16_t wFormatTag;        // 1 = PCM
  uint16_t nChannels;
  uint32_t nSamplesPerSec;
  uint32_t nAvgBytesPerSec;
  uint16_t nBlockAlign;
  uint16_t wBitsPerSample;
};
#pragma pack(pop)

// ─── Buffers ──────────────────────────────────────────────────────────────────

// Max single JPEG frame size (generous for 466×466 Q≈8)
static const size_t JPEG_BUF_SIZE  = 200 * 1024;
// Max audio chunk per frame (64 KB handles 44100 Hz stereo 16-bit @ 10 fps)
static const size_t AUDIO_BUF_SIZE = 64  * 1024;
// Stereo upsample buffer (mono → stereo: 2× the audio chunk)
static const size_t STEREO_BUF_SIZE = AUDIO_BUF_SIZE * 2;

static uint8_t  *s_jpeg_buf   = nullptr;  // JPEG compressed data
static uint8_t  *s_audio_buf  = nullptr;  // raw PCM from AVI
static uint8_t  *s_stereo_buf = nullptr;  // mono→stereo upsampled

// Double buffering for Dual-Core Display Pushing
static uint16_t *s_frame_buf[2]  = {nullptr, nullptr};  
static int s_back_buf_idx = 0;
static int s_front_buf_idx = 0;

static TaskHandle_t s_display_task = nullptr;
static SemaphoreHandle_t s_display_sem = nullptr;
static SemaphoreHandle_t s_display_done_sem = nullptr;

// Allocate a buffer, preferring PSRAM
static uint8_t* psram_alloc(size_t bytes) {
  uint8_t *p = (uint8_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) p = (uint8_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
  return p;
}

// Internal memory allocator for I2S audio data (must be internal RAM for fast DMA!)
static uint8_t* dma_alloc(size_t bytes) {
  return (uint8_t*)heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

// ─── JPEG decode ─────────────────────────────────────────────────────────────

static JPEGDEC s_jpeg;

// Tile callback: copy decoded MCU block into the full-frame PSRAM buffer
static int jpegTileCB(JPEGDRAW *d) {
  const int stride = EXAMPLE_LCD_WIDTH;  // 466
  for (int row = 0; row < d->iHeight; row++) {
    uint16_t *src = d->pPixels + row * d->iWidth;
    uint16_t *dst = s_frame_buf[s_back_buf_idx] + (d->y + row) * stride + d->x;
    memcpy(dst, src, (size_t)d->iWidth * 2);
  }
  return 1;
}

static void displayTask(void *pvParameters) {
  while (true) {
    xSemaphoreTake(s_display_sem, portMAX_DELAY);
    uint16_t *frame = s_frame_buf[s_front_buf_idx];
    // Composite HUD into the same frame before the QSPI push → no flicker
    if (EC11_OverlayActive()) {
      LCD_BlitVolumeOverlay(frame, EC11_GetVolume());
    }
    LCD_addWindow(0, 0, EXAMPLE_LCD_WIDTH - 1, EXAMPLE_LCD_HEIGHT - 1, frame);
    xSemaphoreGive(s_display_done_sem);
  }
}

static void decodeAndDisplay(uint8_t *jpegData, size_t len) {
  if (!s_jpeg.openRAM(jpegData, (int)len, jpegTileCB)) {
    printf("JPEG: open failed\n");
    return;
  }
  s_jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
  s_jpeg.decode(0, 0, 0);
  s_jpeg.close();

  // Wait for the PREVIOUS frame to finish displaying on Core 0
  xSemaphoreTake(s_display_done_sem, portMAX_DELAY);
  
  // Swap buffers
  s_front_buf_idx = s_back_buf_idx;
  s_back_buf_idx = (s_back_buf_idx + 1) % 2;

  // Trigger the display task to draw the newly decoded frame
  xSemaphoreGive(s_display_sem);
}

// ─── I2S / Audio ─────────────────────────────────────────────────────────────

static i2s_chan_handle_t s_i2s_tx      = nullptr;
static bool              s_i2s_ok      = false;
static uint8_t           s_audio_ch    = 1;   // channels from AVI header

static void i2s_teardown() {
  if (s_i2s_tx) {
    i2s_channel_disable(s_i2s_tx);
    i2s_del_channel(s_i2s_tx);
    s_i2s_tx = nullptr;
    s_i2s_ok = false;
  }
}

static bool i2s_setup(uint32_t rate, uint8_t channels, uint8_t bits) {
  i2s_teardown();

  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  
  // Dramatically increase I2S DMA buffer size to prevent choppy audio!
  // The default buffer only holds ~65ms of audio, which stutters instantly if 
  // JPEG decoding lags. We increase this to 10 descriptors * 1023 frames = ~0.5s of audio.
  chan_cfg.dma_desc_num = 10;
  chan_cfg.dma_frame_num = 1023;
  chan_cfg.auto_clear = true;

  if (i2s_new_channel(&chan_cfg, &s_i2s_tx, nullptr) != ESP_OK) {
    printf("I2S: new_channel failed\n");
    return false;
  }

  // Always configure as stereo; mono sources are upsampled before write.
  // External PCM5102A: BCK=18, DIN=17, LCK=16, SCK nc
  i2s_std_config_t cfg = {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(rate),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                  (i2s_data_bit_width_t)bits,
                  I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = (gpio_num_t)I2S_BCK_PIN,
      .ws   = (gpio_num_t)I2S_LRCK_PIN,
      .dout = (gpio_num_t)I2S_DOUT_PIN,
      .din  = I2S_GPIO_UNUSED,
      .invert_flags = { false, false, false },
    },
  };

  // VERY IMPORTANT for PCM5102A: When SCK is tied to GND, the internal PLL
  // requires the Bit Clock (BCK) to be exactly 64x the Word Select (LRCK) frequency.
  // By default, the ESP32 outputs 32x for 16-bit data, causing the PLL to fail and output "clicks".
  // Forcing the slot width to 32 bits generates the 64x BCK ratio!
  cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;
  cfg.slot_cfg.ws_width = 32;

  if (i2s_channel_init_std_mode(s_i2s_tx, &cfg) != ESP_OK) {
    printf("I2S: init_std_mode failed\n");
    i2s_del_channel(s_i2s_tx);
    s_i2s_tx = nullptr;
    return false;
  }
  if (i2s_channel_enable(s_i2s_tx) != ESP_OK) {
    printf("I2S: enable failed\n");
    i2s_del_channel(s_i2s_tx);
    s_i2s_tx = nullptr;
    return false;
  }

  s_i2s_ok = true;
  printf("I2S: %lu Hz  %d ch  %d bit\n", (unsigned long)rate, channels, bits);
  return true;
}

// Software volume: dial 0–100 maps to gain 0 … kMaxGain (was fixed 16×)
// Write audio data to I2S, upsampling mono → stereo if needed, and applying software volume boost.
static bool s_audio_starving = false;

static void audio_write(const uint8_t *data, size_t len) {
  if (!s_i2s_ok || !s_i2s_tx || len == 0) return;

  const float gain = EC11_GetGain();
  uint32_t t_start = millis();
  size_t written = 0;
  
  if (s_audio_ch == 1) {
    // Duplicate mono samples to L and R channels AND boost volume
    if (!s_stereo_buf) return;
    const int16_t *src = (const int16_t*)data;
    int16_t       *dst = (int16_t*)s_stereo_buf;
    size_t samples = len / 2;
    for (size_t i = 0; i < samples; i++) {
      int32_t sample = (int32_t)(src[i] * gain);
      // Hard clip to prevent integer overflow distortion
      if (sample > 32767) sample = 32767;
      if (sample < -32768) sample = -32768;
      
      dst[i * 2    ] = (int16_t)sample;
      dst[i * 2 + 1] = (int16_t)sample;
    }
    
    // Write in small chunks so I2S DMA can ingest it smoothly
    size_t to_write = len * 2;
    uint8_t *ptr = s_stereo_buf;
    while(to_write > 0) {
        size_t chunk = (to_write > 4096) ? 4096 : to_write;
        i2s_channel_write(s_i2s_tx, ptr, chunk, &written, portMAX_DELAY);
        ptr += chunk;
        to_write -= chunk;
    }
  } else {
    // Stereo audio: copy to stereo buffer to apply volume boost
    if (!s_stereo_buf) return;
    const int16_t *src = (const int16_t*)data;
    int16_t       *dst = (int16_t*)s_stereo_buf;
    size_t samples = len / 2;
    for (size_t i = 0; i < samples; i++) {
      int32_t sample = (int32_t)(src[i] * gain);
      // Hard clip to prevent integer overflow distortion
      if (sample > 32767) sample = 32767;
      if (sample < -32768) sample = -32768;
      
      dst[i] = (int16_t)sample;
    }
    
    // Write in small chunks so I2S DMA can ingest it smoothly
    size_t to_write = len;
    uint8_t *ptr = s_stereo_buf;
    while(to_write > 0) {
        size_t chunk = (to_write > 4096) ? 4096 : to_write;
        i2s_channel_write(s_i2s_tx, ptr, chunk, &written, portMAX_DELAY);
        ptr += chunk;
        to_write -= chunk;
    }
  }
  
  uint32_t duration = millis() - t_start;
  // If pushing 66ms of audio took less than 10ms, the DMA buffer was partially empty!
  // We are falling behind and need to skip a video frame to refill the audio buffer.
  if (duration < 10) {
      s_audio_starving = true;
  } else {
      s_audio_starving = false;
  }
}

// ─── RIFF / AVI helpers ───────────────────────────────────────────────────────

static inline uint32_t rd32le(File &f) {
  uint8_t b[4];
  f.read(b, 4);
  return (uint32_t)b[0] | ((uint32_t)b[1]<<8) | ((uint32_t)b[2]<<16) | ((uint32_t)b[3]<<24);
}
static inline uint32_t rd4cc(File &f) {
  uint8_t b[4];
  f.read(b, 4);
  return MK4CC(b[0], b[1], b[2], b[3]);
}

// Parse the hdrl LIST to extract fps, audio params.
// Returns false if the file is malformed.
static bool parse_hdrl(File &f,
                        uint32_t hdrl_end,
                        uint32_t &frame_us,
                        uint32_t &audio_rate,
                        uint8_t  &audio_ch,
                        uint8_t  &audio_bits)
{
  frame_us   = 66666;   // default: 15 fps
  audio_rate = 22050;
  audio_ch   = 1;
  audio_bits = 16;

  // Track which stream index we're currently reading
  int stream_idx = -1;
  char cur_strh_type[4] = {0};

  while ((uint32_t)f.position() + 8 <= hdrl_end) {
    uint32_t fcc  = rd4cc(f);
    uint32_t size = rd32le(f);
    uint32_t dpos = (uint32_t)f.position();
    uint32_t next = dpos + ((size + 1) & ~1u);

    if (fcc == CC_avih && size >= sizeof(MainAVIHeader)) {
      MainAVIHeader h;
      f.read((uint8_t*)&h, sizeof(h));
      frame_us = h.dwMicroSecPerFrame;
      printf("AVI: %lux%lu  %.1f fps  %lu frames\n",
             (unsigned long)h.dwWidth, (unsigned long)h.dwHeight,
             1e6f / (float)h.dwMicroSecPerFrame,
             (unsigned long)h.dwTotalFrames);
    } else if (fcc == CC_LIST) {
      uint32_t ltype = rd4cc(f);
      if (ltype == MK4CC('s','t','r','l')) {
        stream_idx++;
        memset(cur_strh_type, 0, 4);
        // Don't seek; parse inner strl chunks in the next iterations
        continue;
      }
      // Other LIST types inside hdrl: skip
    } else if (fcc == CC_strh && size >= 36) {
      AVIStreamHeader sh;
      f.read((uint8_t*)&sh, sizeof(sh));
      memcpy(cur_strh_type, sh.fccType, 4);
    } else if (fcc == CC_strf) {
      if (memcmp(cur_strh_type, "auds", 4) == 0 && size >= sizeof(WaveFormatEx)) {
        WaveFormatEx wfx;
        f.read((uint8_t*)&wfx, sizeof(wfx));
        audio_rate = wfx.nSamplesPerSec;
        audio_ch   = (uint8_t)wfx.nChannels;
        audio_bits = (uint8_t)wfx.wBitsPerSample;
        printf("Audio: %lu Hz  %d ch  %d bit\n",
               (unsigned long)audio_rate, audio_ch, audio_bits);
      }
    }

    f.seek(next);
  }
  return true;
}

// ─── Frame timing (set from AVI header during Phase 1 scan) ──────────────────

static uint32_t s_frame_us = 66666;  // microseconds per frame (default: 15 fps)

// ─── Public functions ─────────────────────────────────────────────────────────

void AVI_Player_Init() {
  printf("AVI: allocating buffers in PSRAM\n");

  s_jpeg_buf  = psram_alloc(JPEG_BUF_SIZE);
  s_frame_buf[0] = (uint16_t*)psram_alloc(EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT * 2);
  s_frame_buf[1] = (uint16_t*)psram_alloc(EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT * 2);
  
  // VERY IMPORTANT: Audio buffers must be in fast internal DMA RAM, not slow PSRAM!
  s_audio_buf = dma_alloc(AUDIO_BUF_SIZE);
  s_stereo_buf = dma_alloc(STEREO_BUF_SIZE);

  if (!s_jpeg_buf || !s_frame_buf[0] || !s_frame_buf[1] || !s_audio_buf || !s_stereo_buf) {
    printf("AVI: FATAL – buffer allocation failed!\n");
  } else {
    printf("AVI: buffers OK\n");
  }

  // Setup Dual-Core Display Task
  if (!s_display_sem) {
    s_display_sem = xSemaphoreCreateBinary();
    s_display_done_sem = xSemaphoreCreateBinary();
    xSemaphoreGive(s_display_done_sem); // Initially done
    xTaskCreatePinnedToCore(displayTask, "DisplayTask", 4096, nullptr, 3, &s_display_task, 0); // Run on Core 0!
  }
}

void AVI_Player_Play(const char* filename) {
  printf("AVI: opening %s\n", filename);
  File f = SD.open(filename, FILE_READ);
  if (!f) {
    printf("AVI: file not found: %s\n", filename);
    // Let's do a quick directory list of root to debug what IS there
    File root = SD.open("/");
    if (root) {
      printf("Contents of root dir (second try):\n");
      File entry = root.openNextFile();
      while (entry) {
        printf(" - %s\n", entry.name());
        entry = root.openNextFile();
      }
      root.close();
    } else {
      printf("Failed to open root directory!\n");
    }
    
    // Attempt to quickly re-init the SD card because the connection dropped!
    printf("Attempting to recover SD connection...\n");
    SD.end();
    delay(100);
    // We would re-init SDSPI here if we had scope access, 
    // but just let the loop() retry happen for now.
    
    return;
  }

  // Validate RIFF / AVI header
  if (rd4cc(f) != CC_RIFF) { printf("AVI: not a RIFF file\n"); f.close(); return; }
  rd32le(f);  // file size (ignore)
  if (rd4cc(f) != CC_AVI)  { printf("AVI: not an AVI file\n");  f.close(); return; }

  // ── Phase 1: scan hdrl for parameters, locate movi ────────────────────────
  uint32_t movi_start = 0, movi_end = 0;
  f.seek(12);

  while (f.available() > 8) {
    uint32_t fcc  = rd4cc(f);
    uint32_t size = rd32le(f);
    uint32_t dpos = (uint32_t)f.position();
    uint32_t next = dpos + ((size + 1) & ~1u);

    if (fcc == CC_LIST) {
      uint32_t ltype = rd4cc(f);
      if (ltype == CC_hdrl) {
        // Parse audio/video stream headers
        uint32_t frame_us = 66666;
        uint32_t audio_rate = 22050;
        uint8_t  audio_ch = 1, audio_bits = 16;
        parse_hdrl(f, dpos + size, frame_us, audio_rate, audio_ch, audio_bits);
        s_frame_us = frame_us;
        i2s_setup(audio_rate, audio_ch, audio_bits);
        s_audio_ch = audio_ch;
      } else if (ltype == CC_movi) {
        movi_start = (uint32_t)f.position();
        movi_end   = dpos + size;
        break;
      }
    }
    f.seek(next);
    if ((uint32_t)f.position() <= dpos) break;
  }

  if (movi_start == 0) {
    printf("AVI: cannot find 'movi' LIST\n");
    f.close();
    return;
  }
  printf("AVI: movi [%lu – %lu]  target %.1f fps\n",
         (unsigned long)movi_start, (unsigned long)movi_end,
         1e6f / (float)s_frame_us);

  // ── Phase 2: infinite playback loop ────────────────────────────────────────
  uint32_t target_ms    = s_frame_us / 1000;
  if (target_ms == 0) target_ms = 33;

  while (true) {
    f.seek(movi_start);
    uint32_t loop_start  = millis();
    uint32_t frames_played = 0;

    while ((uint32_t)f.position() < movi_end && f.available()) {
      if ((uint32_t)f.position() + 8 > movi_end) break;

      uint32_t fcc  = rd4cc(f);
      uint32_t size = rd32le(f);
      uint32_t dpos = (uint32_t)f.position();
      uint32_t next = dpos + ((size + 1) & ~1u);

      // Guard: sanity-check the chunk boundary
      if (next > movi_end + 2 || size > (uint32_t)f.size()) {
        printf("AVI: bad chunk size %lu at pos %lu – stopping scan\n",
               (unsigned long)size, (unsigned long)dpos);
        break;
      }

      // ── LIST inside movi (e.g. 'rec ') – enter transparently ───────────────
      if (fcc == CC_LIST) {
        rd4cc(f);  // consume list type ('rec ' etc.), then continue reading inner chunks
        continue;
      }

      // ── JUNK / metadata – skip ─────────────────────────────────────────────
      if (fcc == CC_JUNK || fcc == CC_IDIT || fcc == 0) {
        f.seek(next);
        continue;
      }

      // ── idx1 signals end-of-frames
      if (fcc == CC_idx1) break;

      bool is_video = (fcc == CC_00dc || fcc == CC_01dc);
      bool is_audio = (fcc == CC_01wb || fcc == CC_00wb);

      // Standby (dial held 1s): freeze until click wakes (3s input lockout first)
      while (EC11_IsStandby()) {
        vTaskDelay(pdMS_TO_TICKS(50));
      }

      // Check if theme was shaken to change
      if (Theme_HasChanged()) {
          printf("AVI: Theme change detected! Stopping playback.\n");
          break;
      }

      // ── VIDEO frame ────────────────────────────────────────────────────────
      if (is_video) {
        // Dynamic framerate scaling:
        // If the audio buffer is starving (getting empty), we instantly skip decoding this video frame.
        // This gives the CPU 100% of its time to rush forward and read the next Audio chunk,
        // plugging the hole in the buffer and ensuring the music NEVER stops or clicks!
        if (!s_audio_starving) {
            if (size > 0 && size <= JPEG_BUF_SIZE && s_jpeg_buf && s_frame_buf[0]) {
              size_t n = f.read(s_jpeg_buf, size);
              if (n == size) {
                decodeAndDisplay(s_jpeg_buf, size);
              }
            }
        } else {
            // We skipped the video frame! Reset the starving flag so we try to draw the next one.
            s_audio_starving = false;
        }
        
        f.seek(next);
      }
      // ── AUDIO chunk ────────────────────────────────────────────────────────
      else if (is_audio) {
        if (size > 0 && size <= AUDIO_BUF_SIZE && s_audio_buf) {
          size_t n = f.read(s_audio_buf, size);
          if (n == size) audio_write(s_audio_buf, size);
        }
        f.seek(next);
      }
      // ── Unknown chunk – skip ────────────────────────────────────────────────
      else {
        f.seek(next);
      }
    } // end inner while

    printf("AVI: loop\n");
    vTaskDelay(pdMS_TO_TICKS(10));  // brief pause between loops
  }

  // Never reached (infinite loop above), but good practice:
  i2s_teardown();
  f.close();
}
