#ifndef RADAR_MAP_H
#define RADAR_MAP_H

//#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h> 
#include <SPIFFS.h>
#include <time.h>
#include <GxEPD2_3C.h>
#include <PNGdec.h>
#include "DisplayInstance.h"

extern struct tm globalTimeInfo;
extern bool timeInfoValid;

String getRadarUrl(int delay_minutes);
bool fetchLatestRadarImage();
bool isUrlAlreadyDownloaded(const String &url);
void saveDownloadedUrl(const String &url);
bool displaySingaporeMapWithRadarOverlay();
bool displayPNGImage(const char* filename);
uint8_t getHue(uint16_t color);
void precomputeHueTables();
int pngDrawCallback(PNGDRAW *pDraw);
int pngBlueNoiseCallback(PNGDRAW *pDraw);
void* myOpen(const char *filename, int32_t *size);
void myClose(void *handle);
int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length);
int32_t mySeek(PNGFILE *handle, int32_t position);
int getRainCoverPercentage();

#endif
