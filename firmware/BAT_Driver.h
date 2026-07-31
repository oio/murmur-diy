#pragma once
#include <Arduino.h>
#include "I2C_Driver.h"

// Battery voltage via AXP2101 PMIC (replaces ADC GPIO8 on the 1.85 board).
#define AXP2101_ADDR           0x34
#define AXP2101_REG_VBAT_H     0x34
#define AXP2101_REG_VBAT_L     0x35

extern float BAT_analogVolts;

void BAT_Init(void);
float BAT_Get_Volts(void);
