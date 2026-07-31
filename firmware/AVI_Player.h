#pragma once
#include <Arduino.h>

// ─── Public API ───────────────────────────────────────────────────────────────

// Call once after SD_Init() and LCD_Init()
void AVI_Player_Init();

// Play an AVI file (MJPEG video + PCM audio) from the SD card.
// Loops forever; only returns on a fatal file error.
// filename example: "/video.avi"
void AVI_Player_Play(const char* filename);
