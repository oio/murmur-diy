#include "PWR_Key.h"

void PWR_Loop(void) {
  // Power button is wired to AXP2101; no GPIO polling required.
}

void Fall_Asleep(void) {}

void Restart(void) {
  ESP.restart();
}

void Shutdown(void) {
  LCD_Backlight = 0;
  Set_Backlight(0);
}

void PWR_Init(void) {
  // AXP2101 manages battery rails and the side PWR button.
}
