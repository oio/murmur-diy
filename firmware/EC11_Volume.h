#pragma once
#include <Arduino.h>

// Waveshare header UART pins for EC11 rotation
#define EC11_PIN_A   43   // U0TXD  → encoder CLK / A
#define EC11_PIN_B   44   // U0RXD  → encoder DT  / B
// Encoder C (common) → GND

// Switch uses the board's labeled EX0 pad (TCA9554 EXIO0 — NOT a native GPIO!)
#define EC11_EXIO_SW  1   // EXIO_PIN1 == EX0

void EC11_Init(uint8_t start_volume = 70);
void EC11_Loop();
uint8_t EC11_GetVolume();
float   EC11_GetGain();
bool    EC11_OverlayActive();
uint32_t EC11_OverlayUntil();
bool    EC11_IsStandby();   // true when "powered off" (screen dark, muted)
