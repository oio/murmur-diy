#include "Theme_Manager.h"
#include <Preferences.h>
#include "SD_Card.h"
#include <vector>
#include <string.h>

enum ThemeType {
    THEME_CITY = 0,
    THEME_FORREST = 1,
    THEME_WATER = 2,
    THEME_COUNT
};

static ThemeType s_currentTheme = THEME_CITY;
static Preferences s_prefs;
static char s_currentVideoPath[100] = "/video.avi"; // fallback
static volatile bool s_themeChanged = false;

void Theme_Init() {
    s_prefs.begin("murmur", false);
    // Load last theme, default to CITY
    int savedTheme = s_prefs.getInt("theme", THEME_CITY);
    if (savedTheme >= THEME_COUNT || savedTheme < 0) {
        savedTheme = THEME_CITY;
    }
    s_currentTheme = (ThemeType)savedTheme;
    printf("Theme initialized: %d\n", s_currentTheme);
}

const char* Theme_GetNextVideo() {
    char files[50][100];
    const char* prefix = "city";
    if (s_currentTheme == THEME_FORREST) prefix = "forrest";
    if (s_currentTheme == THEME_WATER) prefix = "water";
    
    printf("Scanning SD card for theme: %s\n", prefix);
    uint16_t count = Folder_retrieval("/", prefix, files, 50);
    
    if (count > 0) {
        // Pick random
        int r = esp_random() % count;
        // The Folder_retrieval already prefixes with '/' for root, wait let's check SD_Card.cpp
        // "snprintf(filePath, 100, "%s%s", directory, file.name());"
        // It doesn't actually store filePath in File_Name! It stores file.name().
        snprintf(s_currentVideoPath, sizeof(s_currentVideoPath), "/%s", files[r]);
        printf("Theme selected video: %s\n", s_currentVideoPath);
    } else {
        printf("No files found for theme! Fallback to /video.avi\n");
        strcpy(s_currentVideoPath, "/video.avi");
    }
    return s_currentVideoPath;
}

void Theme_Shake() {
    s_currentTheme = (ThemeType)((s_currentTheme + 1) % THEME_COUNT);
    s_prefs.putInt("theme", (int)s_currentTheme);
    s_themeChanged = true;
    printf("Theme changed (SHAKE!) to: %d\n", s_currentTheme);
}

bool Theme_HasChanged() {
    return s_themeChanged;
}

void Theme_ClearChanged() {
    s_themeChanged = false;
}
