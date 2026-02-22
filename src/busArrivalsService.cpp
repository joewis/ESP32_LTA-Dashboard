#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "busArrivalsService.h"

// LTA API configuration
const String apiKey = LTA_API_KEY; //"X3E8h3+kSQy9yMkjzrps/A=="; 
const char* ltaApiHost = "https://datamall2.mytransport.sg"; 
const char* ltaApiPath = "/ltaodataservice/v3/BusArrival";

// Time-related globals (defined in main sketch)
extern String timestamp;
extern struct tm globalTimeInfo;
extern bool timeInfoValid;

// Global documents (defined in main sketch via configManager)
extern JsonDocument configDoc;
extern JsonDocument busStopsDoc;
extern JsonDocument mergedDoc;
extern JsonDocument busStopArrivals;

/**
 * Processes a single bus data object from the API response
 * and adds it to the target predictions array
 */
void processBusData(JsonArray& target, JsonObject source) {
  if (!source.isNull()) {
    JsonObject bus = target.add<JsonObject>();
    bus["eta"] = source["EstimatedArrival"].as<String>();
    bus["load"] = source["Load"].as<String>();
    bus["type"] = source["Type"].as<String>();
  }
}

/**
 * Fetches bus arrival data for a specific bus stop from the LTA API
 */
String fetchBusArrivals(String busStopCode) {
  HTTPClient http;
  String url = String(ltaApiHost) + ltaApiPath + "?BusStopCode=" + busStopCode;
  http.begin(url);
  http.addHeader("AccountKey", apiKey);
  http.addHeader("accept", "application/json");

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    return http.getString();
  }
  return "{}"; // Return empty object on failure
}

/**
 * Fetches arrival data for all bus stops configured in busStopsDoc
 * Caches the results in busStopArrivals for later use
 */
void fetchAllBusStopArrivals() {
  busStopArrivals.clear();
    
  // Iterate through each bus stop in the bus_stops.json
  for (JsonVariant busStopVariant : busStopsDoc.as<JsonArray>()) {
    String busStopCode = busStopVariant.as<String>();
    String apiResponse = fetchBusArrivals(busStopCode);
    
    // Parse the API response
    JsonDocument apiDoc;
    DeserializationError error = deserializeJson(apiDoc, apiResponse);
    
    // Store the bus stop data in our cache with bus stop code as key
    busStopArrivals[busStopCode] = apiDoc;
    
    // Small delay to be respectful to the API
    delay(100);
  }
  
  Serial.println("Finished fetching all bus stop arrival data");
}

/**
 * Calculates the minutes remaining until a bus arrives
 * Uses the cached global time info for consistency
 */
int calculateMinutesRemaining(String etaTime) {
  if (etaTime == "" || etaTime == "null" || !timeInfoValid) return -1; // No ETA or invalid time
  
  // Parse ETA time (format: "2024-02-20T15:30:00+08:00")
  int etaHour = etaTime.substring(11, 13).toInt();
  int etaMin = etaTime.substring(14, 16).toInt();
  
  // Calculate difference in minutes using cached time info
  int currentMin = globalTimeInfo.tm_hour * 60 + globalTimeInfo.tm_min;
  int etaMinTotal = etaHour * 60 + etaMin;
  
  int diff = etaMinTotal - currentMin;
  return (diff > 0) ? diff : 0; // Return 0 if bus is arriving now/imminently
}

/**
 * Populates destination arrivals from cached API results
 * Merges cached bus arrival data with configuration to create the final merged document
 */
void populateDestinationArrivals() {
  mergedDoc.clear();
  mergedDoc.set(configDoc);
  
  for (JsonObject destination : mergedDoc.as<JsonArray>()) {
    
    for (JsonObject bus_stop : destination["bus_stops"].as<JsonArray>()) {
      String busStopCode = bus_stop["BusStopCode"].as<String>();
      
      // Look up the cached arrival data for this bus stop
      if (!busStopArrivals[busStopCode].is<JsonObject>()) {
        //Serial.printf("No cached data found for bus stop: %s\n", busStopCode.c_str());
        continue;
      }
      
      JsonObject cachedStopData = busStopArrivals[busStopCode];
      
      for (JsonObject service : bus_stop["Services"].as<JsonArray>()) {
        String serviceNo = service["ServiceNo"].as<String>();
        
        // Find matching service in cached data
        for (JsonObject apiService : cachedStopData["Services"].as<JsonArray>()) {
          if (apiService["ServiceNo"] == serviceNo) {
            JsonObject arrivals = service["arrivals"].to<JsonObject>();
            arrivals["updated"] = timestamp; // Use cached timestamp
            
            JsonArray predictions = arrivals["predictions"].to<JsonArray>();
            processBusData(predictions, apiService["NextBus"]);
            processBusData(predictions, apiService["NextBus2"]);
            processBusData(predictions, apiService["NextBus3"]);
            
            break;
          }
        }
      }
    }
  }
  

}


