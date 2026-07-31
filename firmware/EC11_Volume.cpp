#include "EC11_Volume.h"
#include "Display_CO5300.h"
#include "TCA9554PWR.h"

static const float kMaxGain = 16.0f;
static const uint32_t kPowerOffHoldMs = 1000;
static const uint32_t kStandbyLockoutMs = 3000;  // ignore all dial input after standby

static volatile int16_t s_volume = 70;
static volatile int8_t  s_delta  = 0;
static uint32_t s_overlay_until = 0;
static uint8_t  s_last_ab = 0;

static bool     s_sw_down = false;
static uint32_t s_sw_down_ms = 0;
static bool     s_power_off_armed = false;
static bool     s_exio_ok = false;
static volatile bool s_standby = false;
static bool     s_wake_armed = false;
static uint32_t s_standby_lockout_until = 0;

static const int8_t kQuadTable[16] = {
  0, -1,  1,  0,
  1,  0,  0, -1,
 -1,  0,  0,  1,
  0,  1, -1,  0
};

static void IRAM_ATTR onEncoderEdge() {
  uint8_t a = (uint8_t)digitalRead(EC11_PIN_A);
  uint8_t b = (uint8_t)digitalRead(EC11_PIN_B);
  uint8_t ab = (a << 1) | b;
  int8_t step = kQuadTable[(s_last_ab << 2) | ab];
  s_last_ab = ab;
  if (step) s_delta += step;
}

static bool switch_pressed() {
  if (!s_exio_ok) return false;
  return Read_EXIO(EC11_EXIO_SW) == 0;
}

static void discard_encoder() {
  noInterrupts();
  s_delta = 0;
  interrupts();
}

static void enter_standby() {
  printf("EC11: standby – locked 3s, then click dial to wake\n");
  Set_Backlight(0);
  s_standby = true;
  s_wake_armed = false;
  s_power_off_armed = false;
  s_overlay_until = 0;
  s_standby_lockout_until = millis() + kStandbyLockoutMs;
  discard_encoder();
}

bool EC11_IsStandby() {
  return s_standby;
}

void EC11_Init(uint8_t start_volume) {
  if (start_volume > 100) start_volume = 100;
  s_volume = start_volume;
  s_standby = false;

  pinMode(EC11_PIN_A, INPUT_PULLUP);
  pinMode(EC11_PIN_B, INPUT_PULLUP);

  s_last_ab = ((uint8_t)digitalRead(EC11_PIN_A) << 1) | (uint8_t)digitalRead(EC11_PIN_B);
  attachInterrupt(digitalPinToInterrupt(EC11_PIN_A), onEncoderEdge, CHANGE);
  attachInterrupt(digitalPinToInterrupt(EC11_PIN_B), onEncoderEdge, CHANGE);

  uint8_t cfg = Read_REG(TCA9554_CONFIG_REG);
  cfg |= (1u << (EC11_EXIO_SW - 1));
  if (Write_REG(TCA9554_CONFIG_REG, cfg) == 0) {
    s_exio_ok = true;
    printf("EC11: SW on EX0 ready (hold 1s=off, click=on)\n");
  } else {
    printf("EC11: TCA9554 not found – power switch disabled\n");
  }

  s_overlay_until = millis() + 2500;
  printf("EC11: A=TX/GPIO%d  B=RX/GPIO%d  SW=EX0  vol=%d\n",
         EC11_PIN_A, EC11_PIN_B, (int)s_volume);
}

void EC11_Loop() {
  const bool pressed = switch_pressed();
  const uint32_t now = millis();
  const bool locked = (int32_t)(now - s_standby_lockout_until) < 0;

  if (s_standby) {
    // After hold-to-off: ignore wake + volume for lockout window (release bounce, etc.)
    if (locked) {
      s_wake_armed = false;
      s_sw_down = pressed;
      discard_encoder();
      return;
    }
    if (!pressed) {
      s_wake_armed = true;
      s_sw_down = false;
    } else if (s_wake_armed) {
      printf("EC11: wake\n");
      s_standby = false;
      s_wake_armed = false;
      s_sw_down = true;
      s_sw_down_ms = now;
      s_power_off_armed = false;
      discard_encoder();
      Set_Backlight(80);
    }
    discard_encoder();
    return;
  }

  if (pressed) {
    if (!s_sw_down) {
      s_sw_down = true;
      s_sw_down_ms = now;
      s_power_off_armed = false;
    } else if (!s_power_off_armed && (now - s_sw_down_ms) >= kPowerOffHoldMs) {
      s_power_off_armed = true;
      enter_standby();
      return;
    }
  } else {
    s_sw_down = false;
    s_power_off_armed = false;
  }

  if (pressed) {
    discard_encoder();
    return;
  }

  noInterrupts();
  int8_t d = s_delta;
  s_delta = 0;
  interrupts();

  static const int16_t kStep = 5;
  while (d >= 4) {
    d -= 4;
    s_volume = (int16_t)(s_volume + kStep);
    if (s_volume > 100) s_volume = 100;
    s_overlay_until = millis() + 2000;
  }
  while (d <= -4) {
    d += 4;
    s_volume = (int16_t)(s_volume - kStep);
    if (s_volume < 0) s_volume = 0;
    s_overlay_until = millis() + 2000;
  }
  if (d) {
    noInterrupts();
    s_delta += d;
    interrupts();
  }
}

uint8_t EC11_GetVolume() {
  int16_t v = s_volume;
  if (v < 0) return 0;
  if (v > 100) return 100;
  return (uint8_t)v;
}

float EC11_GetGain() {
  if (s_standby) return 0.0f;
  return (kMaxGain * (float)EC11_GetVolume()) / 100.0f;
}

bool EC11_OverlayActive() {
  if (s_standby) return false;
  return (int32_t)(millis() - s_overlay_until) < 0;
}

uint32_t EC11_OverlayUntil() {
  return s_overlay_until;
}
