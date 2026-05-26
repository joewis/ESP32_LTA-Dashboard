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
#include "Utils.h"
#include "blue_noise.h"
#include "fractals.h"
#include "BatteryService.h"


// Deep Sleep Configuration
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP  30
#define GREEN_LED 2

// Define your pins based on your hardware layout
#define EPD_SCL  18  // Your ESP32 SCK pin
#define EPD_SDA  23  // Your ESP32 MOSI pin
#define EPD_BUSY 25
#define EPD_RES  26
#define EPD_DC   27
#define EPD_CS   5

const int potPin=36; // GPIO36 (VP) for potentiometer value reading

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

BatteryService battery;

// ─── RTC Memory: survives deep sleep ───
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int REFRESH_INTERVAL = 2;
RTC_DATA_ATTR bool fetched_github = false;
RTC_DATA_ATTR bool hasRain = false;        // Cached rain state
RTC_DATA_ATTR bool timeWasSet = false;     // Did we ever sync time?
RTC_DATA_ATTR int cachedHour = 0;
RTC_DATA_ATTR int cachedMin = 0;
RTC_DATA_ATTR int cachedWeekday = 0;       // 0=Sunday (tm_wday)
RTC_DATA_ATTR bool batteryWarningShown = false;  // Battery warning rendered yet?


const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;    

IPAddress local_IP(192, 168, 18, 6);
IPAddress gateway(192, 168, 18, 1);  // Usually your router's IP
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // Optional: Google DNS
IPAddress secondaryDNS(8, 8, 4, 4); // Optional: Google DNS

// Connect to WiFi. Returns true when connected.
static bool connectWiFi() {
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure Static IP");
  }
  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false); // Disable WiFi sleep to maintain stable connection for API calls

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
      Serial.println("\nWiFi connection failed after 60 attempts.");
      return false;
    }
  }

  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  return true;
}

//NTP config
const char* ntpServer1 = "sg.pool.ntp.org"; // Regional pool
const char* ntpServer2 = "time.google.com";   // Google global
const char* ntpServer3 = "time.cloudflare.com"; // Cloudflare global
const long  gmtOffset_sec = 8 * 3600;
const int   daylightOffset_sec = 0;

String timestamp = "";
String weekday = "";
String prettydate = "";

// Global time structure - fetched once via NTP
struct tm globalTimeInfo;

// Initialize time info once via NTP
static bool initializeTimeInfo() {
  const int MAX_ATTEMPTS = 5;
  int attempt = 0;
  unsigned long delayMs = 1000;

  while (attempt < MAX_ATTEMPTS) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);
    if (getLocalTime(&globalTimeInfo)) {

      char timeBuf[10];
      strftime(timeBuf, sizeof(timeBuf), "%H:%M", &globalTimeInfo);
      timestamp = String(timeBuf);

      char dayBuf[10];
      strftime(dayBuf, sizeof(dayBuf), "%A", &globalTimeInfo);
      weekday = String(dayBuf);

      char dateBuf[17];
      strftime(dateBuf, sizeof(dateBuf), "%a %d %b %H:%M", &globalTimeInfo);
      prettydate = String(dateBuf);

      // Cache into RTC memory
      cachedHour = globalTimeInfo.tm_hour;
      cachedMin = globalTimeInfo.tm_min;
      cachedWeekday = globalTimeInfo.tm_wday;
      timeWasSet = true;

      Serial.printf("Time initialized: %s, %s\n", timestamp.c_str(), weekday.c_str());
      Serial.printf("Pretty Date: %s\n", prettydate.c_str());
      return true;
    }
    attempt++;
    Serial.printf("Failed to get time from NTP (attempt %d/%d). Retrying in %lu ms...\n", attempt, MAX_ATTEMPTS, delayMs);
    delay(delayMs);
    delayMs = min(delayMs * 2, 8000UL);
  }

  Serial.println("Failed to get time from NTP after multiple attempts");
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
  display.epd2.selectFastFullUpdate(true);
}

// Initialize SPIFFS. Returns true on success.
static bool initSpiffs() {
  if (!initSPIFFS()) {
    Serial.println("SPIFFS initialization failed");
    return false;
  }
  return true;
}


// Consolidated cleanup before sleep: disconnect WiFi, end Serial, power off display and SPIFFS
static void cleanupBeforeSleep() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.end();
  display.powerOff();
  display.end();
  SPIFFS.end();
}

static void goToSleep(){
  Serial.printf("Going to sleep for %d seconds...\n", TIME_TO_SLEEP);
  display.hibernate();
  cleanupBeforeSleep();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}


// ─── Use cached time when WiFi was skipped ───
static void useCachedTime() {
  // Increment ~1 min per 2 wake cycles (30s * 2 = 60s)
  cachedMin++;
  if (cachedMin >= 60) {
    cachedMin = 0;
    cachedHour++;
    if (cachedHour >= 24) {
      cachedHour = 0;
      cachedWeekday = (cachedWeekday + 1) % 7;
    }
  }

  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", cachedHour, cachedMin);
  timestamp = String(buf);

  const char* days[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
  weekday = String(days[cachedWeekday % 7]);

  Serial.printf("Cached time: %s, %s\n", timestamp.c_str(), weekday.c_str());
}

// ─── Helpers ───
static int currentHour() { return cachedHour; }
static bool isDaytime()   { return cachedHour >= 6 && cachedHour < 20; }


// ─── Radar ───
static void downloadRadarMap() {
  int delay_minutes = 10;
  bool downloaded = false;

  while (!downloaded) {
    String radarUrl = getRadarUrl(delay_minutes);
    downloaded = isUrlAlreadyDownloaded(radarUrl);
    if (!downloaded) {
      if (fetchRadarImage(radarUrl)) {
        saveDownloadedUrl(radarUrl);
        downloaded = true;
      } else {
        delay_minutes += 5;
        delay(1000);
      }
    }
  }
}


// ─── Bus display ───
static void handleBusDisplay() {
  fetched_github = loadConfigurationFiles();
  Serial.println("Displaying bus arrival times");
  fetchAllBusStopArrivals();
  populateDestinationArrivals();
  renderBusDisplayPaged();
}


// ─── Main display decision ───
static void updateDisplay() {
  initDisplay();

  bool hadWiFi = (WiFi.status() == WL_CONNECTED);

  // Rain check — only tries download if WiFi connected
  int rainCover = 0;
  if (hadWiFi && SPIFFS.exists("/radar.png")) {
    downloadRadarMap();
    rainCover = getRainCoverPercentage();
    hasRain = (rainCover > 10);
  }
  rainCover = hasRain ? 60 : 0;  // Use cached rain state when offline

  Serial.printf("Boot: %d, WiFi: %d, Rain: %d\n", bootCount, (int)hadWiFi, (int)hasRain);

  // ─── Decision logic ───
  if (hasRain && (bootCount % 4 == 0) && hadWiFi) {
    precomputeHueTables();
    displaySingaporeMapWithRadarOverlay();

  } else if (isDaytime() && (bootCount % 2 == 0)) {
    if (hadWiFi) {
      handleBusDisplay();
    } else {
      // Offline daytime fallback: just show time
      display.setFullWindow();
      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
        u8g2Fonts.setFont(u8g2_font_fub30_tr);
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        u8g2Fonts.setCursor(30, 80);
        u8g2Fonts.printf("%s", timestamp.c_str());
        displayBatteryLevel(timestamp);
      } while (display.nextPage());
    }

  } else if (bootCount % 10 == 0) {
    displayMandelbrot();
  }
  // else: no display update this cycle
}


// ─── Determines if this wake cycle needs WiFi ───
static bool cycleNeedsWiFi() {
  if (isDaytime()) {
    // Even boot cycles = bus display
    if (bootCount % 2 == 0) return true;
  }
  // First boot without a time sync ever: try WiFi
  if (!timeWasSet) return true;
  // Nighttime: no WiFi needed
  return false;
}


// ─── Render battery critical screen (once, persists until charge) ───
static void showBatteryCritical() {
  if (batteryWarningShown) {
    return;
  }

  initDisplay();
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    u8g2Fonts.setFont(u8g2_font_fub30_tr);
    u8g2Fonts.setForegroundColor(GxEPD_RED);
    u8g2Fonts.setCursor(30, 60);
    u8g2Fonts.printf("BATTERY CRITICAL");

    u8g2Fonts.setFont(u8g2_font_helvB18_tr);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setCursor(30, 100);
    u8g2Fonts.printf("Please charge");

    u8g2Fonts.setFont(u8g2_font_helvB12_tr);
    u8g2Fonts.setCursor(30, 135);
    u8g2Fonts.printf("%d%%", battery.getPercentage());

    // Static battery icon at top-right (no displayBatteryLevel call
    // to avoid triggering voltage cutoff or drawing the time)
    u8g2Fonts.setFont(u8g2_font_battery19_tn);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setFontMode(1);
    String batStr = String(getBatteryIcon(battery.getPercentage()));
    int batW = u8g2Fonts.getUTF8Width(batStr.c_str());
    int lh = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent();
    u8g2Fonts.setCursor(display.width() - batW, lh + u8g2Fonts.getFontDescent());
    u8g2Fonts.printf("%s", batStr.c_str());
  } while (display.nextPage());

  batteryWarningShown = true;
  Serial.println("Battery critical — warning displayed");
}


void setup() {
  // Green LED status
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  delay(100);
  digitalWrite(GREEN_LED, HIGH);

  Serial.begin(115200);

  pinMode(potPin, INPUT);
  battery.begin(potPin, 2.17f, 3300.0f);

  bootCount++;

  // Seed cached time (increments ~1 minute every 2 wake cycles)
  useCachedTime();

  // ─── Battery check — gates everything else ───
  if (battery.isCritical()) {
    if (!batteryWarningShown) {
      showBatteryCritical();
    }
    // Critical: skip WiFi, skip display updates, just go back to sleep
    goToSleep();
    return;  // never reached, but clear intent
  }

  // ─── Battery recovered — reset warning flag ───
  if (batteryWarningShown && !battery.isCritical()) {
    batteryWarningShown = false;
    Serial.println("Battery recovered — resuming normal operation");
  }

  // ─── Conditional WiFi ───
  bool needsWiFi = cycleNeedsWiFi();

  if (needsWiFi) {
    initSpiffs();
    if (connectWiFi()) {
      initializeTimeInfo();  // NTP sync — updates cached time
    }
  }

  precomputeHueTables();

  // ─── Display decision ───
  bool shouldDisplay = false;
  if (isDaytime() && (bootCount % 2 == 0)) shouldDisplay = true;
  if (bootCount % 10 == 0) shouldDisplay = true;
  if (bootCount <= 3) shouldDisplay = true;  // Debug: always show first 3 boots

  if (shouldDisplay) {
    initDisplay();
    updateDisplay();
  }

  goToSleep();
}


void loop() {
  // All work in setup() + deep sleep
}
