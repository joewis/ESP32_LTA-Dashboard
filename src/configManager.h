#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPIFFS.h>

// JSON Documents (extern declarations)
extern JsonDocument configDoc;
extern JsonDocument timeSlotsDoc;
extern JsonDocument mergedDoc;
extern JsonDocument busStopsDoc;
extern JsonDocument busStopArrivals;

// Configuration file structure
struct ConfigFile {
  const char* url;
  const char* localPath;
  JsonDocument* doc;
};

// Configuration files array (extern declaration)
extern ConfigFile configFiles[];
extern const int numConfigFiles;

// Function declarations
bool initSPIFFS();
bool loadLocalJson(const char* filePath, JsonDocument& doc);
bool saveLocalJson(const char* filePath, const String& jsonString);
bool fetchJson(const ConfigFile& config, bool fetched_github);
bool loadConfigurationFiles();
int getEastPM25();

#endif
