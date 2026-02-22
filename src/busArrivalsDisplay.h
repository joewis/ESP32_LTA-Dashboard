#ifndef BUS_ARRIVALS_DISPLAY_H
#define BUS_ARRIVALS_DISPLAY_H

#include <ArduinoJson.h>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <vector>
#include "DisplayInstance.h"

// Global display objects
//extern GxEPD2_3C<GxEPD2_420c_GDEY042Z98, GxEPD2_420c_GDEY042Z98::HEIGHT > display;
extern U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// Global documents and data
extern JsonDocument mergedDoc;
extern JsonDocument timeSlotsDoc;
extern String timestamp;
extern String weekday;

// Display state
extern int yPos;
extern std::vector<int> displayedIndices;
//extern std::vector<int> destinationsToDisplay;  // Pre-determined destinations for paged display

// Function declarations
void displayDestinationHeader(JsonObject destination);
void displayBusPredictions(JsonObject destination);
//void prepareDestinationsForDisplay();  // Determine which destinations to display (including random fill)
//void drawPreparedDestinations();  // Draw destinations from the predetermined list
void displayDestinations();
void displayRandomDestination();
void renderBusDisplayPaged();

#endif // BUS_ARRIVALS_DISPLAY_H
