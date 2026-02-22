#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <time.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include "configManager.h"
#include "radarMap.h"
#include "busArrivalsService.h"
#include "busArrivalsDisplay.h"
#include "DisplayInstance.h"
#include "blue_noise.h"


// Deep Sleep Configuration
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP  30
#define GREEN_LED 2

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int REFRESH_INTERVAL = 2;
RTC_DATA_ATTR bool fetched_github = false;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;    

IPAddress local_IP(192, 168, 18, 6);
IPAddress gateway(192, 168, 18, 1);  // Usually your router's IP
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // Optional: Google DNS
IPAddress secondaryDNS(8, 8, 4, 4); // Optional: Google DNS

//NTP config
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 8 * 3600;
const int   daylightOffset_sec = 0;

String timestamp = "";
String weekday = "";
String prettydate = "";

// Global time structure - fetched once via NTP
struct tm globalTimeInfo;
bool timeInfoValid = false;


// Initialize time info once via NTP
bool initializeTimeInfo() {
  const int MAX_ATTEMPTS = 5;
  int attempt = 0;
  unsigned long delayMs = 1000; // start with 1s backoff

  while (attempt < MAX_ATTEMPTS) {
    if (getLocalTime(&globalTimeInfo)) {
      timeInfoValid = true;

      // Set global timestamp and weekday
      char timeBuf[10];
      strftime(timeBuf, sizeof(timeBuf), "%H:%M", &globalTimeInfo);
      timestamp = String(timeBuf);

      char dayBuf[10];
      strftime(dayBuf, sizeof(dayBuf), "%A", &globalTimeInfo);
      weekday = String(dayBuf);

      char dateBuf[17];
      strftime(dateBuf, sizeof(dateBuf), "%a %d %b %H:%M", &globalTimeInfo);
      prettydate = String(dateBuf);

      Serial.printf("Time initialized: %s, %s\n", timestamp.c_str(), weekday.c_str());
      Serial.printf("Pretty Date: %s\n", prettydate.c_str());
      return true;
    }

    attempt++;
    Serial.printf("Failed to get time from NTP (attempt %d/%d). Retrying in %lu ms...\n", attempt, MAX_ATTEMPTS, delayMs);
    delay(delayMs);
    // exponential backoff but cap at 8s
    delayMs = min(delayMs * 2, 8000UL);
  }

  Serial.println("Failed to get time from NTP after multiple attempts");
  timeInfoValid = false;
  return false;
}

void initDisplay() {
  display.init(115200);
  display.setFullWindow();
  display.setRotation(0);
  u8g2Fonts.begin(display);
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
  u8g2Fonts.setFont(u8g2_font_fub20_tr);
  u8g2Fonts.setFontMode(1);
  display.fillScreen(GxEPD_WHITE);
  
}

void displayTime(){
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);
  u8g2Fonts.setFont(u8g2_font_helvB12_tr);
  int textwidth = u8g2Fonts.getUTF8Width(prettydate.c_str());
  int lineHeight = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
  int yPos = lineHeight + u8g2Fonts.getFontDescent();
  u8g2Fonts.setCursor(display.width() - textwidth, yPos);
  u8g2Fonts.printf("%s", prettydate.c_str());
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
  int dateWidth = u8g2Fonts.getUTF8Width(prettydate.c_str());
  int lineHeight = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
  int yPos = lineHeight + u8g2Fonts.getFontDescent();
  u8g2Fonts.setCursor(display.width() - textwidth - dateWidth - 10 , yPos);
  u8g2Fonts.printf("%s", pm25Str.c_str());
}


// Initialize pins, serial and other on-device hardware
static void initHardware() {
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);

  delay(1000);
  digitalWrite(GREEN_LED, HIGH);
  Serial.begin(115200);

}

// Initialize SPIFFS. Returns true on success.
static bool initSpiffs() {
  if (!initSPIFFS()) {
    Serial.println("SPIFFS initialization failed");
    return false;
  }
  return true;
}

// Connect to WiFi. Returns true when connected.
static bool connectWiFi() {
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure Static IP");
  }
  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  Serial.print("Connecting to WiFi");

  int connectAttempts = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    connectAttempts++;

    // Re-issue begin after 30s in case of silent handshake failure
    if (connectAttempts == 30) {
      Serial.println("\nRetrying WiFi.begin()...");
      WiFi.begin(ssid, password);
    }

    // Hardware reset after 60s to clear any radio stack hangs
    if (connectAttempts >= 60) {
      Serial.println("\nWiFi failed. Restarting ESP32...");
      ESP.restart();
    }
  }

  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  return true;
}

void displayJuliaSet() {
    uint8_t ditherTable[33];
    for (int i = 0; i <= 32; i++) {
        ditherTable[i] = (uint8_t)((i * i * 255) / (1024)); // Scaled for 32 iterations
    }

    int w = display.width();
    int h = display.height();
    int stepSize = 2; 

    // Randomize c, but steer toward "interesting" regions
    //float cRe = (float)random(-700, 300) / 1000.0f;
    //float cIm = (float)random(-600, 600) / 1000.0f;

    // Pick a random angle and a specific radius range
    float angle = (float)random(0, 360) * 0.0174533f; // Degrees to Radians
    // Radius 0.6 to 0.8 is the "Golden Zone" for thin, detailed sets
    float radius = (float)random(600, 850) / 1000.0f; 

    float cRe = cos(angle) * radius;
    float cIm = sin(angle) * radius;

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE); 

        for (int y = 0; y < h; y += stepSize) {
            float startIm = (y - h / 2.0f) / (0.5f * 1.2f * h);
            for (int x = 0; x < w; x += stepSize) {
                float zRe = 1.5f * (x - w / 2.0f) / (0.5f * 1.2f * w);
                float zIm = startIm;
                int i;

                for (i = 0; i < 32; i++) {
                    float r2 = zRe * zRe;
                    float i2 = zIm * zIm;
                    if (r2 + i2 > 4.0f) break;
                    zIm = 2.0f * zRe * zIm + cIm;
                    zRe = r2 - i2 + cRe;
                }

                uint8_t noise = pgm_read_byte(&blueNoise64[(y % 64) * 64 + (x % 64)]);
                
                if (i == 32) {
                    // --- THE FIX: Dither the Red interior ---
                    // Use the final magnitude to create internal detail
                    float mag = sqrt(zRe * zRe + zIm * zIm);
                    uint8_t innerThreshold = (uint8_t)(mag * 120); // Adjust 120 for "vein" density
                    
                    if (noise > innerThreshold) {
                        display.fillRect(x, y, stepSize, stepSize, GxEPD_RED);
                    } else {
                        // This creates white/black veins inside the red blob
                        display.fillRect(x, y, stepSize, stepSize, GxEPD_BLACK);
                    }
                } else if (i > 3) {
                    // Exterior black dithering
                    if (noise < ditherTable[i]) {
                        display.fillRect(x, y, stepSize, stepSize, GxEPD_BLACK);
                    }
                }
            }
            if (y % 20 == 0) yield();
        }
    } while (display.nextPage());
}


// Consolidated cleanup before sleep: disconnect WiFi, end Serial, power off display and SPIFFS
static void cleanupBeforeSleep() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  Serial.end();
  display.powerOff();
  display.end();

  // Unmount SPIFFS if mounted
  SPIFFS.end();
}

// Extracted helper: handle radar display path
static void downloadRadarMap() {
  
  int delay_minutes = 10;
  bool downloaded = false;

  while (!downloaded) {
    String radarUrl = getRadarUrl(delay_minutes);
    downloaded = isUrlAlreadyDownloaded(radarUrl);
    if (!downloaded) {
      if (fetchRadarImage(radarUrl)) {
        // 3. Mark URL as downloaded
        saveDownloadedUrl(radarUrl);
        downloaded = true;
      } else {
        delay_minutes += 5;
        delay(1000); // be nice to the server
      }
    }
  }

}


static void handleBusDisplay() {
  Serial.println("Displaying bus arrival times");

  fetchAllBusStopArrivals();
  populateDestinationArrivals();

  displayDestinations();
  displayRandomDestination();
  //renderBusDisplayPaged();
}

void updateDisplay() {
  initDisplay(); 
  
  downloadRadarMap();
  int rainCover = getRainCoverPercentage();

  //print current bootcount modulo refresh interval and rain cover for debugging
  Serial.printf("Boot Count: %d, Refresh Interval: %d, Rain Cover: %d%%\n", bootCount-1, 2*REFRESH_INTERVAL, rainCover);

  // Decide which display to show. 
  if ((rainCover > 10 && (bootCount-1) % (2*REFRESH_INTERVAL) == 0)) { 
    displaySingaporeMapWithRadarOverlay();
    REFRESH_INTERVAL = 2; // every 5 minutes during radar display hours
  } else if((timestamp >= "06:00" && timestamp <= "20:00")){
    handleBusDisplay();
    REFRESH_INTERVAL = 2; // every 1 minute during bus arrival display hours
  } else {
    // Night Mode / No Rain
    displayJuliaSet();
    REFRESH_INTERVAL = 10; // Wake up much less often (every 30 mins) to save battery
  }

  // Common footer elements
  displayTime();
  displayPMI25();

  display.display();
  display.hibernate();
}

void setup() {
  initHardware();

  if (bootCount++ % REFRESH_INTERVAL == 0) {
    if (initSpiffs()) {
      if (connectWiFi()) {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        initializeTimeInfo();
        precomputeHueTables();
        fetched_github = loadConfigurationFiles();
        updateDisplay();
      }
    }
  }

  Serial.printf("Going to sleep for %d seconds...\n", TIME_TO_SLEEP);

  // Cleanup peripherals and connections before sleeping
  cleanupBeforeSleep();

  // we startup every 30 seconds to prevent brownout but only refresh bus arrival times every 1 min to reduce battery drain
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {}
