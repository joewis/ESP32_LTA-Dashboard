#include "Utils.h"
#include "DisplayInstance.h"
#include <BatteryService.h>

void displayTime(const String& timeString){
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setFont(u8g2_font_helvB12_tr);
  int textwidth = u8g2Fonts.getUTF8Width(timeString.c_str());
  int lineHeight = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
  int yPos = lineHeight + u8g2Fonts.getFontDescent();
  u8g2Fonts.setCursor(display.width() - textwidth, yPos);
  u8g2Fonts.printf("%s", timeString.c_str());
}

void displayPMI25() {
  int pm25 = getEastPM25();
  if (pm25 > 50) {
    u8g2Fonts.setForegroundColor(GxEPD_RED);
  } else{
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  }
  u8g2Fonts.setFont(u8g2_font_helvB12_tr);
  String pm25Str = "PM25: " + String(pm25);
  int textwidth = u8g2Fonts.getUTF8Width(pm25Str.c_str());
  int lineHeight = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
  int yPos = lineHeight + u8g2Fonts.getFontDescent();
  u8g2Fonts.setCursor(display.width() - textwidth , yPos);
  u8g2Fonts.printf("%s", pm25Str.c_str());
}

extern BatteryService battery;

// Function to get battery icon character based on percentage
char getBatteryIcon(int percentage) {
    if (percentage <= 10) return '0';     // No bar
    if (percentage <= 25) return '1';     // 1 bars
    if (percentage <= 50) return '2';     // 2 bars
    if (percentage <= 70) return '3';     // 3 bars
    if (percentage <= 90) return '4';     // 4 bars
    return '5';                            //5 bars (full)
}

void displayBatteryLevel() {
  int batteryLevel = battery.getPercentage();
  String batteryStr="";
  if (battery.isCritical()) {
    u8g2Fonts.setForegroundColor(GxEPD_RED);
    //batteryStr = "Battery Level Critical! : " + String(batteryLevel) + "%";
    
  } else {
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    //batteryStr = "Battery: " + String(batteryLevel) + "%";
  }

  batteryStr = String(getBatteryIcon(batteryLevel));
  //batteryStr = "012345";
  
  u8g2Fonts.setFont(u8g2_font_battery19_tn);
  
  //u8g2Fonts.setFont(u8g2_font_helvB12_tr);
  u8g2Fonts.setFontMode(1);
  int textwidth = u8g2Fonts.getUTF8Width(batteryStr.c_str());
  int lineHeight = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
  int yPos = lineHeight + u8g2Fonts.getFontDescent();
  u8g2Fonts.setCursor(display.width() - textwidth, yPos);
  u8g2Fonts.printf("%s", batteryStr.c_str());

  if (batteryLevel < 3.1) {
    esp_deep_sleep_start(); // Force deep sleep if battery is critically low to prevent battery damage
  }
}