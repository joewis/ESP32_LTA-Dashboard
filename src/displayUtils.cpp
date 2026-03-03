#include "displayUtils.h"
#include "DisplayInstance.h"

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

float getBatteryLevel(int potPin = 36) {
    int analogValue = analogRead(potPin);
    float voltage = (3300*analogValue / 4095)/1000.0f*2.17f; // Convert to voltage and apply voltage divider factor (2.11)
    return voltage;
}

void displayBatteryLevel() {
  float batteryLevel = getBatteryLevel();
  String batteryStr="";
  if (batteryLevel < 3.3) {
    u8g2Fonts.setForegroundColor(GxEPD_RED);
    batteryStr = "Battery Level Critial! : " + String(batteryLevel, 2) + "V";
    
  } else {
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    batteryStr = "Battery: " + String(batteryLevel, 2) + "V";
  }
  
  u8g2Fonts.setFont(u8g2_font_helvB12_tr);
  int textwidth = u8g2Fonts.getUTF8Width(batteryStr.c_str());
  int lineHeight = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
  int yPos = lineHeight + u8g2Fonts.getFontDescent();
  u8g2Fonts.setCursor(display.width() - textwidth, yPos);
  u8g2Fonts.printf("%s", batteryStr.c_str());

  if (batteryLevel < 3.1) {
    esp_deep_sleep_start(); // Force deep sleep if battery is critically low to prevent battery damage
  }
}