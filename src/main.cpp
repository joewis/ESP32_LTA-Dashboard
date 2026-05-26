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
RTC_DATA_ATTR bool hasRain = false;       // Cached: was there rain last radar check?
RTC_DATA_ATTR bool timeWasSet = false;    // Cached: do we have valid NTP time?
RTC_DATA_ATTR int cachedHour = 0;          // Cached: last known hour
RTC_DATA_ATTR int cachedMin = 0;          // Cached: last known minute
RTC_DATA_ATTR int cachedWeekday = 0;      // Cached: last known weekday (tm_wday 0=Sun)


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
const char* ntpServer1 = "sg.pool.ntp.org";
const char* ntpServer2 = "time.google.com";
const char* ntpServer3 = "time.cloudflare.com";
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
  // Build a reasonable approximation from cached values
  // Increment minute every ~2 wake cycles (30s * 2 = 60s)
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

// ─── Helpers for decision logic ───
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

// ─── Decides what to show. Callers (setup) already sorted WiFi+time. ───
static void updateDisplay() {
  initDisplay();

  // If we have WiFi, consider downloading fresh radar
  bool hadWiFi = (WiFi.status() == WL_CONNECTED);

  // Rain check
  int rainCover = 0;
  if (hadWiFi && SPIFFS.exists("/radar.png")) {
    downloadRadarMap();
    rainCover = getRainCoverPercentage();
    hasRain = (rainCover > 10);
  }
  rainCover = hasRain ? 60 : 0;  // Use cached rain state if no fresh data

  Serial.printf("Boot Count: %d, Rain: hasRain=%d\n", bootCount, (int)hasRain);

  // ─── Decision ───
  if (hasRain && (bootCount % 4 == 0) && hadWiFi) {
    // Got fresh radar while connected — redraw with overlay
    precomputeHueTables();
    displaySingaporeMapWithRadarOverlay();

  } else if (isDaytime() && (bootCount % 2 == 0)) {
    if (hadWiFi) {
      // Fresh bus data cycle
      handleBusDisplay();
    } else {
      // Offline daytime: show what we can (cached bus data or just time/date)
      initDisplay();
      // Minimal fallback: just show time on blank screen
      u8g2Fonts.setFont(u8g2_font_fub30_tr);
      u8g2Fonts.setForegroundColor(GxEPD_BLACK);
      display.setFullWindow();
      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
        u8g2Fonts.setCursor(30, 80);
        u8g2Fonts.printf("%s", timestamp.c_str());
        displayBatteryLevel();
      } while (display.nextPage());
    }

  } else if (bootCount % 10 == 0) {
    // Fractal — no WiFi needed ever
    displayMandelbrot();
  }
  // else: no display update this cycle — just go back to sleep
}

// ─── Determines if this wake cycle needs WiFi ───
static bool cycleNeedsWiFi() {
  // Daytime: even cycles may need bus data
  if (isDaytime()) {
    // Bus cycles (every 2nd) OR rain cycles (every 4th)
    if ((bootCount % 2 == 0)) return true;
  }
  // Nighttime: no WiFi needed (fractals are local)
  // First boot of a day: try WiFi to re-sync in case we were off
  if (!timeWasSet) return true;

  return false;
}

void setup() {

  // Initialize green LED pin for status indication
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  delay(1000);
  digitalWrite(GREEN_LED, HIGH);

  Serial.begin(115200);

  pinMode(potPin, INPUT);
  battery.begin(potPin, 2.17f, 3300.0f);

  bootCount++;

  // Use cached time as baseline (increments approx 30s each wake)
  useCachedTime();

  // ─── Conditional WiFi ───
  bool needsWiFi = cycleNeedsWiFi();

  if (needsWiFi) {
    initSpiffs();
    if (connectWiFi()) {
      initializeTimeInfo();   // NTP syncs; updates cached time
      // Now we have fresh time and can do bus/radar work
    }
  }
  precomputeHueTables();

  // ─── Display logic ───
  bool shouldDisplay = false;

  // Always try to show something if cycle is due
  if (isDaytime()) {
    // Bus: every 2nd cycle
    // Rain radar: every 4th cycle
    if (bootCount % 2 == 0) shouldDisplay = true;
  }
  // Fractal: every 10th cycle
  if (bootCount % 10 == 0) shouldDisplay = true;
  // Always show on first few boots for debugging
  if (bootCount <= 3) shouldDisplay = true;

  if (shouldDisplay) {
    initDisplay();
    updateDisplay();
  }

  // Battery critical check (from Utils)
  if (battery.isCritical()) {
    displayBatteryLevel();  // One last update
  }

  goToSleep();
}

// ─── Loop is dead — everything runs through setup + deep sleep ───
void loop() {
  // Unused. All work is done in setup(), then deep sleep.
}

