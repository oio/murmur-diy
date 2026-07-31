#pragma once
#include <stdint.h>

#define EXAMPLE_LCD_WIDTH                   (466)
#define EXAMPLE_LCD_HEIGHT                  (466)
#define EXAMPLE_LCD_COLOR_BITS              (16)

#define Backlight_MAX   100

// Waveshare ESP32-S3-Touch-AMOLED-1.75 QSPI pinout
#define ESP_PANEL_LCD_SPI_IO_CS             (12)
#define ESP_PANEL_LCD_SPI_IO_SCK            (38)
#define ESP_PANEL_LCD_SPI_IO_DATA0          (4)
#define ESP_PANEL_LCD_SPI_IO_DATA1          (5)
#define ESP_PANEL_LCD_SPI_IO_DATA2          (6)
#define ESP_PANEL_LCD_SPI_IO_DATA3          (7)
#define EXAMPLE_LCD_PIN_NUM_RST             (39)

// CO5300 panels on this module need a 6 px column offset
#define EXAMPLE_LCD_COL_OFFSET              (6)

extern uint8_t LCD_Backlight;

void LCD_Init();
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t *color);
// Paint a centered volume HUD into an RGB565 frame buffer (no flicker).
void LCD_BlitVolumeOverlay(uint16_t *frame, uint8_t volume);
void LCD_DrawVolumeOverlay(uint8_t volume);  // boot splash helper

void Backlight_Init();
void Set_Backlight(uint8_t Light);
