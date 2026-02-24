#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "busArrivalsDisplay.h"
#include "busArrivalsService.h"
#include "displayUtils.h"

// Display state tracking
int yPos = 0;
std::vector<int> displayedIndices;
std::vector<int> destinationsToDisplay;  // Pre-determined destinations for paged display

// External references
extern JsonDocument mergedDoc;
extern JsonDocument timeSlotsDoc;
extern String timestamp;
extern String weekday;
extern U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

/**
 * Displays the header for a destination (name and line separator)
 */
void displayDestinationHeader(JsonObject destination) {
  // Display destination header
  u8g2Fonts.setFont(u8g2_font_helvB18_tr);
  int lineHeight = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
  
  // Only increment yPos if we're not starting at the top
  if (yPos == 0) {
    yPos += u8g2Fonts.getFontAscent();
  }// else {
   // yPos += lineHeight; // Add space between sections
  //}
  

  u8g2Fonts.setCursor(0, yPos);
  u8g2Fonts.printf("%s", destination["destination_name"].as<const char*>());
  display.drawLine(0, yPos - lineHeight, display.width(), yPos - lineHeight, GxEPD_BLACK);
}

/**
 * Displays bus predictions for a single destination
 * Shows all services and their estimated arrival times with load indicators
 */
void displayBusPredictions(JsonObject destination) {
  int textwidth;
  int lineHeight;
  int spacing;

  String eta;
  String load;
  String type;
  String predictionText;

  displayDestinationHeader(destination);
  //int16_t yPos = u8g2Fonts.getCursorY(); // Reference to the current Y position

  // Display all bus stops and services for this destination
  u8g2Fonts.setFont(u8g2_font_fub30_tr);
  u8g2Fonts.setFontMode(1);
  
  lineHeight = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
  spacing = u8g2Fonts.getUTF8Width(" ") / 2;

  yPos += lineHeight;
  
  for (JsonObject bus_stop : destination["bus_stops"].as<JsonArray>()) {
    if (yPos > display.height()) break;
    
    for (JsonObject service : bus_stop["Services"].as<JsonArray>()) {
      if (yPos > display.height()) break;
      
      if (service["arrivals"].is<JsonObject>()) {
        u8g2Fonts.setCursor(5, yPos);
        u8g2Fonts.printf("%s:", service["ServiceNo"].as<const char*>());
        
        JsonArray predictions = service["arrivals"]["predictions"];
        int xPos = u8g2Fonts.getUTF8Width("555: ");
        
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
            display.fillRect(xPos, yPos - lineHeight + spacing, textwidth, lineHeight - spacing / 2, GxEPD_BLACK);
            u8g2Fonts.setForegroundColor(GxEPD_WHITE);
          }
          
          if (load != "SEA") {
            // Set color based on load
            u8g2Fonts.setForegroundColor(GxEPD_RED);
          } 

          u8g2Fonts.setCursor(xPos + spacing, yPos);
          u8g2Fonts.printf("%s", predictionText);
          xPos += textwidth;
          
          u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        }
        yPos += lineHeight;
      }
    }
  }
}

/**
 * Displays destinations based on current time slot matching
 * First tries to find destinations that match the current weekday and time
 */
//  void displayDestinations() {
//   bool foundActiveDestination = false;
  
//    // First try to find matching destination based on weekday and time
//    for (int i = 0; i < mergedDoc.size(); i++) {
//      JsonObject destination = mergedDoc[i];
//      bool shouldDisplay = false;
    
//      // Check each time slot ID for this destination
//      for (JsonVariant slotId : destination["time_slot_ids"].as<JsonArray>()) {
//        // Find the time slot definition
//        for (JsonObject time_slot : timeSlotsDoc.as<JsonArray>()) {
//          if (time_slot["id"].as<String>() == slotId.as<String>()) {
//            //Serial.printf("Checking time slot: %s\n", time_slot["id"].as<String>());
          
//            // Check weekday match
//            bool weekdayMatch = false;
//            for (JsonVariant day : time_slot["days_of_week"].as<JsonArray>()) {
//              if (day.as<String>() == weekday) {
//                weekdayMatch = true;
//                break;
//              }
//            }
//            if (!weekdayMatch) continue;
          
//            // Check time match
//            if (timestamp >= time_slot["start_time"].as<String>() && 
//                timestamp <= time_slot["end_time"].as<String>()) {
//              shouldDisplay = true;
//              break;
//            }
//          }
//        }
//        if (shouldDisplay) break;
//      }
    
//      if (shouldDisplay) {
//        foundActiveDestination = true;
//        displayBusPredictions(destination);
//        displayedIndices.push_back(i); // Mark this destination as displayed
//      }
//    }
//  }

// /**
//  * Fills remaining screen space with random destinations not yet displayed
//  */
//  void displayRandomDestination() {
//    // If no matching destination found or screen not full, display random non-duplicate ones
//    while (yPos < display.height() && displayedIndices.size() < mergedDoc.size()) {
//      int destinationCount = mergedDoc.size();
//      if (destinationCount > 0) {
//        int randomIndex;
//        bool isUnique;
      
//        // Keep generating random indices until we find one we haven't displayed yet
//        do {
//          randomIndex = random(destinationCount);
//          isUnique = true;
//          for (int displayedIndex : displayedIndices) {
//            if (randomIndex == displayedIndex) {
//              isUnique = false;
//              break;
//            }
//          }
//        } while (!isUnique && displayedIndices.size() < destinationCount);
      
//        // If we've displayed all destinations, break the loop
//        if (!isUnique) break;
      
//        displayBusPredictions(mergedDoc[randomIndex]);
//        displayedIndices.push_back(randomIndex); // Mark this destination as displayed
//      }
//    }
//  }



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

void renderBusDisplayPaged() {
  prepareDestinationsForDisplay();

  display.setFullWindow();
  display.firstPage();
  do {
    // 1. Reset state for EVERY page iteration
    int localYPos = 0; // Use local variable for paged updates to avoid global state modification
    yPos = 0; // Also reset global for consistency
    //display.setCursor(0, 0); // Reset cursor to top-left for every page
    display.fillScreen(GxEPD_WHITE);
    
    // 2. Re-apply font settings inside the loop
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);

    // 3. Draw the list
    for (int index : destinationsToDisplay) {
      // Temporarily assign global yPos to local for this page rendering
      yPos = localYPos;
      displayBusPredictions(mergedDoc[index]);
      localYPos = yPos; // Update localYPos after drawing
    }
    

  } while (display.nextPage());
}

