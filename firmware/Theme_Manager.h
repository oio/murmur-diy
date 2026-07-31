#pragma once
#include <stdbool.h>

void Theme_Init();
void Theme_Shake();
const char* Theme_GetNextVideo();
bool Theme_HasChanged();
void Theme_ClearChanged();
