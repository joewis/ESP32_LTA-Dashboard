#include "configManager.h"
#include <Arduino.h>

// JSON Documents (definitions)
JsonDocument configDoc;
JsonDocument timeSlotsDoc;
JsonDocument mergedDoc; // Larger doc for merged data
JsonDocument busStopsDoc;
JsonDocument busStopArrivals; // Cache for all bus stop arrival data

// Define configuration files
ConfigFile configFiles[] = {
  {
    "https://raw.githubusercontent.com/joewis/LTABusDashboard/refs/heads/main/bus_stops.json",
    "/bus_stops.json",
    &busStopsDoc
  },
  {
    "https://raw.githubusercontent.com/joewis/LTABusDashboard/refs/heads/main/destinations.json",
    "/destinations.json",
    &configDoc
  },
  {
    "https://raw.githubusercontent.com/joewis/LTABusDashboard/refs/heads/main/time_slots.json",
    "/time_slots.json",
    &timeSlotsDoc
  }
};

const int numConfigFiles = sizeof(configFiles) / sizeof(ConfigFile);

// Initialize SPIFFS
bool initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    //Serial.println("SPIFFS initialization failed!");
    return false;
  }
  //Serial.println("SPIFFS initialized successfully");
  return true;
}

// Load JSON from local file
bool loadLocalJson(const char* filePath, JsonDocument& doc) {
  if (!SPIFFS.exists(filePath)) {
    //Serial.printf("Local file not found: %s\n", filePath);
    return false;
  }
  
  File file = SPIFFS.open(filePath, "r");
  if (!file) {
    //Serial.printf("Failed to open file: %s\n", filePath);
    return false;
  }
  
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    //Serial.printf("Failed to parse JSON from %s: %s\n", filePath, error.c_str());
    return false;
  }
  
  //Serial.printf("Successfully loaded JSON from %s\n", filePath);
  return true;
}

// Save JSON to local file
bool saveLocalJson(const char* filePath, const String& jsonString) {
  File file = SPIFFS.open(filePath, "w");
  if (!file) {
    //Serial.printf("Failed to create file: %s\n", filePath);
    return false;
  }
  
  size_t written = file.print(jsonString);
  file.close();

  if (written > 0) {
    //Serial.printf("Successfully saved JSON to %s (%d bytes)\n", filePath, written);
  } else {
    //Serial.printf("Failed to write to file: %s\n", filePath);
  }
  
  return written > 0;
}

bool fetchJson(const ConfigFile& config, bool fetched_github) {
  if (!fetched_github) {
    HTTPClient http;
    http.begin(config.url);
    
    if (http.GET() == HTTP_CODE_OK) {
      String response = http.getString();
      http.end();
      saveLocalJson(config.localPath, response);
    } else {
      http.end();
    }
  } else {
    Serial.println("Loading local files");
  }

  return loadLocalJson(config.localPath, *config.doc);
}

// Load all configuration files
bool loadConfigurationFiles() {
  bool allSuccess = true;
  bool fetched_github = false;
  
  for (int i = 0; i < numConfigFiles; i++) {
    Serial.printf("Loading configuration file %d/%d: %s\n", i+1, numConfigFiles, configFiles[i].url);
    
    if (!fetchJson(configFiles[i], fetched_github)) {
      Serial.printf("Failed to load configuration file: %s\n", configFiles[i].url);
      allSuccess = false;
    }
  }
  
  return allSuccess;
}

int getEastPM25() {
    HTTPClient http;
    // The API endpoint for real-time PM2.5
    const char* url = "https://api-open.data.gov.sg/v2/real-time/api/pm25";
    
    http.begin(url);
    int httpCode = http.GET();
    int pm25Value = -1; // Default error value

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // DynamicJsonDocument or JsonDocument depending on your ArduinoJson version
        // The response is medium-sized, 2048-3072 bytes is usually safe.
        JsonDocument doc; 
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            // Path: data -> items[0] -> readings -> pm25_one_hourly -> east
            pm25Value = doc["data"]["items"][0]["readings"]["pm25_one_hourly"]["east"] | -1;
            Serial.printf("Current East PM2.5: %d\n", pm25Value);
        } else {
            Serial.print("PM2.5 JSON Parse Failed: ");
            Serial.println(error.c_str());
        }
    } else {
        Serial.printf("PM2.5 HTTP Request Failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    return pm25Value;
}