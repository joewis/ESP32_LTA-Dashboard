#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "configManager.h"

// External declarations needed by the display utility functions
extern U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
//extern String prettydate;

// Function declarations
void displayTime(const String& timeString);
void displayPMI25();
float getBatteryLevel(int potPin);
void displayBatteryLevel(const String& timeStr = "");
char getBatteryIcon(int percentage);

#endif // DISPLAY_UTILS_H