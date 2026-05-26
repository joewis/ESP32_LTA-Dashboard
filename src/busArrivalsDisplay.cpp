#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "busArrivalsDisplay.h"
#include "busArrivalsService.h"
#include "Utils.h"

std::vector<int> displayedIndices;
std::vector<int> destinationsToDisplay;  // Pre-determined destinations for paged display

// External references
extern JsonDocument mergedDoc;
extern JsonDocument timeSlotsDoc;
extern String timestamp;
extern String weekday;
extern U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

int8_t lineHeight(){
  return u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();  
}

int16_t newLine(){
  u8g2Fonts.setCursor(0, u8g2Fonts.getCursorY() + lineHeight());
  return u8g2Fonts.getCursorY();
}


/**
 * Displays the header for a destination (name and line separator)
 */
void displayDestinationHeader(JsonObject destination) {
  u8g2Fonts.setFont(u8g2_font_helvB18_tr);
  newLine();
  u8g2Fonts.printf("%s", destination["destination_name"].as<const char*>());
  display.drawLine(0, u8g2Fonts.getCursorY(), display.width(), u8g2Fonts.getCursorY(), GxEPD_BLACK);
}

/**
 * Displays bus predictions for a single destination
 * Shows all services and their estimated arrival times with load indicators
 */
void displayBusPredictions(JsonObject destination) {
  int textwidth;
  int spacing;

  String eta;
  String load;
  String type;
  String predictionText;

  displayDestinationHeader(destination);
  
  // Display all bus stops and services for this destination
  u8g2Fonts.setFont(u8g2_font_fub30_tr);
  u8g2Fonts.setFontMode(1);
  spacing = u8g2Fonts.getUTF8Width(" ") / 2;
  
  // Loop through bus stops
  for (JsonObject bus_stop : destination["bus_stops"].as<JsonArray>()) {
    if (u8g2Fonts.getCursorY() > display.height()) break;
    
    // Loop through services at this bus stop
    for (JsonObject service : bus_stop["Services"].as<JsonArray>()) {
      if (u8g2Fonts.getCursorY() > display.height()) break;
      
      // Only display if there are predictions available
      if (service["arrivals"].is<JsonObject>()) {
        newLine();
        u8g2Fonts.setCursor(5, u8g2Fonts.getCursorY());
        u8g2Fonts.printf("%s:", service["ServiceNo"].as<const char*>());
        
        JsonArray predictions = service["arrivals"]["predictions"];
        int xPos = u8g2Fonts.getUTF8Width("555: ");
        
        // Loop through predictions for this service
        for (JsonObject pred : predictions) {
          eta = pred["eta"].as<String>();
          load = pred["load"].as<String>();
          type = pred["type"].as<String>();
          int mins = calculateMinutesRemaining(eta);

          if (mins >= 5) {
            predictionText = String(mins) + "m";
          } else {
            predictionText = "--";
          }

          textwidth = u8g2Fonts.getUTF8Width("00m ");
          if (xPos + textwidth > display.width()) break;
                    
          if (type == "DD") {
            // Double-decker - black text on white background with a box
            display.fillRect(xPos, u8g2Fonts.getCursorY() - lineHeight() + spacing, textwidth, lineHeight() - spacing / 2, GxEPD_BLACK);
            u8g2Fonts.setForegroundColor(GxEPD_WHITE);
          }
          
          if (load != "SEA") {
            // Set color based on load
            u8g2Fonts.setForegroundColor(GxEPD_RED);
          } 

          u8g2Fonts.setCursor(xPos + spacing, u8g2Fonts.getCursorY());
          u8g2Fonts.printf("%s", predictionText);
          xPos += textwidth;
          
          u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        }
      }
    }
  }
}


 void prepareDestinationsForDisplay() {
  destinationsToDisplay.clear();
  displayedIndices.clear();

  // 1. Identify "Active" destinations (Time/Day match)
  for (int i = 0; i < mergedDoc.size(); i++) {
    JsonObject destination = mergedDoc[i];
    bool shouldDisplay = false;

    for (JsonVariant slotId : destination["time_slot_ids"].as<JsonArray>()) {
      for (JsonObject time_slot : timeSlotsDoc.as<JsonArray>()) {
        if (time_slot["id"].as<String>() == slotId.as<String>()) {
          // Weekday check
          bool weekdayMatch = false;
          for (JsonVariant day : time_slot["days_of_week"].as<JsonArray>()) {
            if (day.as<String>() == weekday) { weekdayMatch = true; break; }
          }
          if (!weekdayMatch) continue;

          // Time check
          if (timestamp >= time_slot["start_time"].as<String>() && 
              timestamp <= time_slot["end_time"].as<String>()) {
            shouldDisplay = true;
            break;
          }
        }
      }
      if (shouldDisplay) break;
    }

    if (shouldDisplay) {
      destinationsToDisplay.push_back(i);
      displayedIndices.push_back(i);
    }
  }

  // 2. Fill remaining slots with random ones until we have enough to fill the screen
  // (Assuming ~3-4 destinations fit on your 4.2" screen)
  while (destinationsToDisplay.size() < mergedDoc.size() && destinationsToDisplay.size() < 4) {
    int randomIndex = random(mergedDoc.size());
    
    // Check if already in list
    bool exists = false;
    for (int idx : destinationsToDisplay) {
      if (idx == randomIndex) { exists = true; break; }
    }
    
    if (!exists) {
      destinationsToDisplay.push_back(randomIndex);
    }
  }
}

void renderBusDisplay(){
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);

    // 1. Reset cursor to top-left for this page
    u8g2Fonts.setCursor(0, 0);
    display.setCursor(0, 0);

    // 3. Draw the list
    for (int index : destinationsToDisplay) {
      displayBusPredictions(mergedDoc[index]);
    }
    
    displayBatteryLevel(timestamp); // Overlay battery level on top of the display

}

void renderBusDisplayPaged() {
  prepareDestinationsForDisplay();

  display.setFullWindow();
  display.firstPage();
  do {

    renderBusDisplay(); // Render the current page content

  } while (display.nextPage());
}

