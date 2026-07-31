#include "BAT_Driver.h"

float BAT_analogVolts = 0;

void BAT_Init(void) {
  // AXP2101 is on the shared I2C bus (initialized by I2C_Init).
}

float BAT_Get_Volts(void) {
  uint8_t hi = 0, lo = 0;
  // I2C_Read returns 0 on success, non-zero on failure (legacy bool API)
  if (I2C_Read(AXP2101_ADDR, AXP2101_REG_VBAT_H, &hi, 1) ||
      I2C_Read(AXP2101_ADDR, AXP2101_REG_VBAT_L, &lo, 1)) {
    BAT_analogVolts = 0;
    return BAT_analogVolts;
  }
  // AXP2101 VBAT is 14-bit, 1 mV/LSB
  const uint16_t raw = ((uint16_t)(hi & 0x3F) << 8) | lo;
  BAT_analogVolts = raw / 1000.0f;
  return BAT_analogVolts;
}
