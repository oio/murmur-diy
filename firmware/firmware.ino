/**
 * VideoPlayer.ino – Looping MJPEG video player for Waveshare
 * ESP32-S3-Touch-AMOLED-1.75 (466×466 CO5300).
 *
 * Put your AVI file (named "video.avi") in the root of the TF/SD card.
 * Connect the speaker to the board's MX1.25 speaker connector.
 *
 * See README.md for full setup, video conversion, and flashing instructions.
 *
 * Required libraries (Arduino Library Manager):
 *   JPEGDEC                 by Larry Bank
 *   GFX Library for Arduino by moononournation
 */

#include "Display_CO5300.h"
#include "SD_Card.h"
#include "TCA9554PWR.h"
#include "I2C_Driver.h"
#include "PWR_Key.h"
#include "BAT_Driver.h"
#include "AVI_Player.h"
#include "IMU_Shake.h"
#include "Theme_Manager.h"

static void DriverTask(void *) {
  while (true) {
    PWR_Loop();
    BAT_Get_Volts();
    if (IMU_CheckShake()) {
      Theme_Shake();
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void setup() {
  // Use USB CDC for serial, avoid mapping hardware UART to GPIO 1/3 (SD card pins)
  Serial.begin(115200);
  // Serial.setTxTimeoutMs(0);
  delay(200);
  printf("\n=== VideoPlayer boot (466 AMOLED) ===\n");

  PWR_Init();
  I2C_Init(); 
  BAT_Init(); 
  IMU_Init();
  Theme_Init();

  LCD_Init();
  Backlight_Init();
  Set_Backlight(80);

  // Set RXD (GPIO 44) high to output 3.3V
  pinMode(44, OUTPUT);
  digitalWrite(44, HIGH);

  SD_Init();
  AVI_Player_Init();

  xTaskCreatePinnedToCore(DriverTask, "DriverTask", 2048, nullptr, 2, nullptr, 1);

  static uint16_t white_line[EXAMPLE_LCD_WIDTH];
  memset(white_line, 0xFF, sizeof(white_line));
  for (int y = 0; y < EXAMPLE_LCD_HEIGHT; y++)
    LCD_addWindow(0, y, EXAMPLE_LCD_WIDTH - 1, y, white_line);

  float volts = BAT_Get_Volts();
  int pct = (int)((volts - 3.3f) / (4.2f - 3.3f) * 100.0f);
  if (pct < 0)   pct = 0;
  if (pct > 100) pct = 100;
  printf("Battery: %d%%  %.2f V\n", pct, volts);
  
  vTaskDelay(pdMS_TO_TICKS(2000));

  printf("Starting video\n");
}

void loop() {
  const char* nextVid = Theme_GetNextVideo();
  AVI_Player_Play(nextVid);

  if (!Theme_HasChanged()) {
    printf("Video error or finished. Reloading...\n");
  } else {
    Theme_ClearChanged();
    printf("Theme switched! Loading new theme video...\n");
  }

  static uint16_t blk[EXAMPLE_LCD_WIDTH];
  memset(blk, 0, sizeof(blk));
  for (int y = 0; y < EXAMPLE_LCD_HEIGHT; y++)
    LCD_addWindow(0, y, EXAMPLE_LCD_WIDTH - 1, y, blk);

  vTaskDelay(pdMS_TO_TICKS(100)); // small delay before next video starts
}
