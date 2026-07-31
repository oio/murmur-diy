#include "Display_CO5300.h"

#include <Arduino_GFX_Library.h>
#include <esp_heap_caps.h>
#include <stdio.h>
#include <string.h>

uint8_t LCD_Backlight = 50;

static Arduino_DataBus *bus = nullptr;
static Arduino_CO5300 *gfx = nullptr;

void LCD_Init() {
  bus = new Arduino_ESP32QSPI(
      ESP_PANEL_LCD_SPI_IO_CS, ESP_PANEL_LCD_SPI_IO_SCK, ESP_PANEL_LCD_SPI_IO_DATA0,
      ESP_PANEL_LCD_SPI_IO_DATA1, ESP_PANEL_LCD_SPI_IO_DATA2, ESP_PANEL_LCD_SPI_IO_DATA3);

  // col_offset1=6 avoids a dark strip on the Waveshare 466×466 CO5300 module
  gfx = new Arduino_CO5300(bus, EXAMPLE_LCD_PIN_NUM_RST, 0 /* rotation */,
                           EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT, EXAMPLE_LCD_COL_OFFSET, 0, 0, 0);

  if (!gfx->begin()) {
    printf("CO5300: gfx->begin() failed\n");
    return;
  }
  gfx->fillScreen(RGB565_BLACK);
  printf("CO5300: %dx%d ready\n", EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT);
}

void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t *color) {
  if (!gfx || !color) return;
  if (Xend >= EXAMPLE_LCD_WIDTH) Xend = EXAMPLE_LCD_WIDTH - 1;
  if (Yend >= EXAMPLE_LCD_HEIGHT) Yend = EXAMPLE_LCD_HEIGHT - 1;
  if (Xstart > Xend || Ystart > Yend) return;

  const int16_t w = (int16_t)(Xend - Xstart + 1);
  const int16_t h = (int16_t)(Yend - Ystart + 1);
  // Video / JPEG pipelines produce little-endian RGB565; GFX converts for the panel.
  gfx->draw16bitRGBBitmap((int16_t)Xstart, (int16_t)Ystart, color, w, h);
}

// Tiny 5x7 glyphs (column bitmaps, bit0 = top row)
static const uint8_t kFont5x7[][5] = {
  {0x3E,0x51,0x49,0x45,0x3E}, // 0
  {0x00,0x42,0x7F,0x40,0x00}, // 1
  {0x42,0x61,0x51,0x49,0x46}, // 2
  {0x21,0x41,0x45,0x4B,0x31}, // 3
  {0x18,0x14,0x12,0x7F,0x10}, // 4
  {0x27,0x45,0x45,0x45,0x39}, // 5
  {0x3C,0x4A,0x49,0x49,0x30}, // 6
  {0x01,0x71,0x09,0x05,0x03}, // 7
  {0x36,0x49,0x49,0x49,0x36}, // 8
  {0x06,0x49,0x49,0x29,0x1E}, // 9
  {0x00,0x00,0x00,0x00,0x00}, // space (10)
  {0x1F,0x20,0x40,0x20,0x1F}, // V (11)
  {0x3E,0x41,0x41,0x41,0x3E}, // O (12)
  {0x7F,0x40,0x40,0x40,0x40}, // L (13)
};

static int glyphIndex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c == ' ') return 10;
  if (c == 'V' || c == 'v') return 11;
  if (c == 'O' || c == 'o') return 12;
  if (c == 'L' || c == 'l') return 13;
  return 10;
}

static void blitFillRect(uint16_t *frame, int x, int y, int w, int h, uint16_t color) {
  const int W = EXAMPLE_LCD_WIDTH;
  const int H = EXAMPLE_LCD_HEIGHT;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > W) w = W - x;
  if (y + h > H) h = H - y;
  if (w <= 0 || h <= 0) return;
  for (int row = 0; row < h; row++) {
    uint16_t *dst = frame + (y + row) * W + x;
    for (int col = 0; col < w; col++) dst[col] = color;
  }
}

static void blitChar(uint16_t *frame, int x, int y, char c, int scale, uint16_t color) {
  const uint8_t *g = kFont5x7[glyphIndex(c)];
  for (int col = 0; col < 5; col++) {
    uint8_t bits = g[col];
    for (int row = 0; row < 7; row++) {
      if (bits & (1 << row)) {
        blitFillRect(frame, x + col * scale, y + row * scale, scale, scale, color);
      }
    }
  }
}

static void blitText(uint16_t *frame, int x, int y, const char *text, int scale, uint16_t color) {
  for (const char *p = text; *p; p++) {
    blitChar(frame, x, y, *p, scale, color);
    x += (5 + 1) * scale;
  }
}

void LCD_BlitVolumeOverlay(uint16_t *frame, uint8_t volume) {
  if (!frame) return;
  if (volume > 100) volume = 100;

  // Centered panel on 466×466
  const int panel_w = 280;
  const int panel_h = 110;
  const int panel_x = (EXAMPLE_LCD_WIDTH - panel_w) / 2;
  const int panel_y = (EXAMPLE_LCD_HEIGHT - panel_h) / 2;

  const uint16_t black = 0x0000;
  const uint16_t white = 0xFFFF;
  const uint16_t track = 0x4208;  // dark track, no outline

  blitFillRect(frame, panel_x, panel_y, panel_w, panel_h, black);

  char buf[16];
  snprintf(buf, sizeof(buf), "VOL %d", volume);
  const int scale = 4;
  const int text_w = (int)strlen(buf) * (5 + 1) * scale - scale;
  const int text_x = panel_x + (panel_w - text_w) / 2;
  const int text_y = panel_y + 18;
  blitText(frame, text_x, text_y, buf, scale, white);

  const int bar_x = panel_x + 24;
  const int bar_y = panel_y + 72;
  const int bar_w = panel_w - 48;
  const int bar_h = 18;
  blitFillRect(frame, bar_x, bar_y, bar_w, bar_h, track);
  int fill = (int)((uint32_t)volume * bar_w / 100);
  if (fill > 0) {
    blitFillRect(frame, bar_x, bar_y, fill, bar_h, white);
  }
}

// Kept for boot splash before video starts
void LCD_DrawVolumeOverlay(uint8_t volume) {
  if (!gfx) return;
  static uint16_t *tmp = nullptr;
  if (!tmp) {
    tmp = (uint16_t *)heap_caps_malloc(EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT * 2,
                                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  if (!tmp) return;
  memset(tmp, 0, EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT * 2);
  LCD_BlitVolumeOverlay(tmp, volume);
  LCD_addWindow(0, 0, EXAMPLE_LCD_WIDTH - 1, EXAMPLE_LCD_HEIGHT - 1, tmp);
}

void Backlight_Init() {
  // Brightness is a CO5300 register, not a PWM backlight pin.
  Set_Backlight(LCD_Backlight);
}

void Set_Backlight(uint8_t Light) {
  if (Light > Backlight_MAX) {
    printf("Set Backlight parameters in the range of 0 to 100\n");
    Light = Backlight_MAX;
  }
  LCD_Backlight = Light;
  if (!gfx) return;
  // Map 0–100 → 0–255 panel brightness
  gfx->setBrightness((uint8_t)((uint16_t)Light * 255u / 100u));
}
