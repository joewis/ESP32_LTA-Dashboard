#ifndef BUS_ARRIVALS_DISPLAY_H
#define BUS_ARRIVALS_DISPLAY_H

#include <ArduinoJson.h>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <vector>
#include "DisplayInstance.h"

// Global display objects
extern U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// Global documents and data
extern JsonDocument mergedDoc;
extern JsonDocument timeSlotsDoc;
extern String timestamp;
extern String weekday;

// Display state
extern std::vector<int> displayedIndices;

// Function declarations
void displayDestinationHeader(JsonObject destination);
void displayBusPredictions(JsonObject destination);

void displayDestinations();
void displayRandomDestination();
void renderBusDisplayPaged();

int8_t lineHeight();
int16_t newLine();

#endif // BUS_ARRIVALS_DISPLAY_H
