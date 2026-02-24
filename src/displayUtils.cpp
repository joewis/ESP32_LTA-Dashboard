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