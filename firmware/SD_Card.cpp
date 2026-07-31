#include "SD_Card.h"
#include <SPI.h>

bool SDCard_Flag = 0;
bool SDCard_Finish = 0;

uint16_t SDCard_Size = 0;
uint16_t Flash_Size = 0;

// VERY IMPORTANT: Use SPI3 (HSPI equivalent on S3) for the SD card so it doesn't conflict 
// with the AMOLED QSPI driver which likely grabs SPI2 (FSPI).
SPIClass SDSPI(HSPI); 

void SD_Init() {
  SDSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);

  // Now that we are safely on HSPI, we can run at full 40MHz speed
  // to prevent video buffering lag!
  if (!SD.begin(SD_CS_PIN, SDSPI, 40000000)) { 
    printf("SD card initialization failed!\r\n");
    return;
  }
  printf("SD card initialization successful!\r\n");

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    printf("No SD card attached\r\n");
    return;
  }

  printf("SD Card Type: ");
  if (cardType == CARD_MMC) {
    printf("MMC\r\n");
  } else if (cardType == CARD_SD) {
    printf("SDSC\r\n");
  } else if (cardType == CARD_SDHC) {
    printf("SDHC\r\n");
  } else {
    printf("UNKNOWN\r\n");
  }

  uint64_t totalBytes = SD.totalBytes();
  uint64_t usedBytes = SD.usedBytes();
  SDCard_Size = totalBytes / (1024 * 1024);
  printf("Total space: %llu\n", totalBytes);
  printf("Used space: %llu\n", usedBytes);
  printf("Free space: %llu\n", totalBytes - usedBytes);

  printf("\n--- SD CARD ROOT CONTENTS ---\n");
  File root = SD.open("/");
  if (root) {
    File entry = root.openNextFile();
    while (entry) {
      printf("  %s\n", entry.name());
      entry = root.openNextFile();
    }
    root.close();
  }
  printf("-----------------------------\n");
  
  printf("TEST: Trying to open video.avi directly after init...\n");
  File testF = SD.open("/video.avi", FILE_READ);
  if (testF) {
      printf("TEST: SUCCESS! File opened. Size: %lu\n", (unsigned long)testF.size());
      testF.close();
  } else {
      printf("TEST: FAILED to open video.avi!\n");
  }
}

bool File_Search(const char* directory, const char* fileName) {
  File Path = SD.open(directory);
  if (!Path) {
    printf("Path: <%s> does not exist\r\n", directory);
    return false;
  }
  File file = Path.openNextFile();
  while (file) {
    if (strcmp(file.name(), fileName) == 0) {
      if (strcmp(directory, "/") == 0)
        printf("File '%s%s' found in root directory.\r\n", directory, fileName);
      else
        printf("File '%s/%s' found in root directory.\r\n", directory, fileName);
      Path.close();
      return true;
    }
    file = Path.openNextFile();
  }
  if (strcmp(directory, "/") == 0)
    printf("File '%s%s' not found in root directory.\r\n", directory, fileName);
  else
    printf("File '%s/%s' not found in root directory.\r\n", directory, fileName);
  Path.close();
  return false;
}

uint16_t Folder_retrieval(const char* directory, const char* fileExtension, char File_Name[][100], uint16_t maxFiles) {
  File Path = SD.open(directory);
  if (!Path) {
    printf("Path: <%s> does not exist\r\n", directory);
    return 0;
  }

  uint16_t fileCount = 0;
  char filePath[100];
  File file = Path.openNextFile();
  while (file && fileCount < maxFiles) {
    if (!file.isDirectory() && strstr(file.name(), fileExtension)) {
      strncpy(File_Name[fileCount], file.name(), sizeof(File_Name[fileCount]));
      if (strcmp(directory, "/") == 0) {
        snprintf(filePath, 100, "%s%s", directory, file.name());
      } else {
        snprintf(filePath, 100, "%s/%s", directory, file.name());
      }
      printf("File found: %s\r\n", filePath);
      fileCount++;
    }
    file = Path.openNextFile();
  }
  Path.close();
  if (fileCount > 0) {
    printf("Retrieved %d files\r\n", fileCount);
    return fileCount;
  }
  printf("No files with extension '%s' found in directory: %s\r\n", fileExtension, directory);
  return 0;
}

void Flash_test() {
  printf("/********** RAM Test**********/\r\n");
  uint32_t flashSize = ESP.getFlashChipSize();
  Flash_Size = flashSize / 1024 / 1024;
  printf("Flash size: %d MB \r\n", flashSize / 1024 / 1024);
  printf("/******* RAM Test Over********/\r\n\r\n");
}
