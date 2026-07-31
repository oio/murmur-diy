#pragma once
#include "Arduino.h"
#include <cstring>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Waveshare ESP32-S3-Touch-AMOLED-1.75 TF card (SPI)
#define SD_CS_PIN       41
#define SD_MOSI_PIN     1
#define SD_MISO_PIN     3
#define SD_SCK_PIN      2

extern uint16_t SDCard_Size;
extern uint16_t Flash_Size;

void SD_Init();
void Flash_test();

bool File_Search(const char* directory, const char* fileName);
uint16_t Folder_retrieval(const char* directory, const char* fileExtension, char File_Name[][100],uint16_t maxFiles);
