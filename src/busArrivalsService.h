#ifndef BUS_ARRIVALS_SERVICE_H
#define BUS_ARRIVALS_SERVICE_H

#include <ArduinoJson.h>

// LTA API configuration
extern const String apiKey;
extern const char* ltaApiHost;
extern const char* ltaApiPath;

// Global cache for bus stop arrival data
extern JsonDocument busStopArrivals;

// Global documents used by bus arrivals service
extern JsonDocument configDoc;
extern JsonDocument busStopsDoc;
extern JsonDocument mergedDoc;

// Time-related globals
extern String timestamp;
extern struct tm globalTimeInfo;
extern bool timeInfoValid;

// Function declarations for bus arrivals service
void processBusData(JsonArray& target, JsonObject source);
String fetchBusArrivals(String busStopCode);
void fetchAllBusStopArrivals();
void populateDestinationArrivals();
int calculateMinutesRemaining(String etaTime);

#endif // BUS_ARRIVALS_SERVICE_H
