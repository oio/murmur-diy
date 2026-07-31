#pragma once
#include "Arduino.h"
#include "Display_CO5300.h"

// On the 1.75 AMOLED board, power on/off is handled by the AXP2101 PMIC
// (hardware PWR button). Do not reuse the 1.85 GPIO 6/7 hold pins — those
// GPIOs are QSPI data lines on this module.

#define Device_Sleep_Time    10
#define Device_Restart_Time  15
#define Device_Shutdown_Time 20

void Fall_Asleep(void);
void Shutdown(void);
void Restart(void);

void PWR_Init(void);
void PWR_Loop(void);
