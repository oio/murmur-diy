#pragma once
#include <Wire.h>

// Waveshare ESP32-S3-Touch-AMOLED-1.75
#define I2C_SCL_PIN       14
#define I2C_SDA_PIN       15

void I2C_Init(void);

bool I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length);
bool I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length);
