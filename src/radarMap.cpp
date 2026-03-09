#include "radarMap.h"
#include "blue_noise.h"
#include "Utils.h"

// Global constant for the filename
const char* DOWNLOADED_URL_FILE = "/downloaded.txt";
const char* RADAR_IMAGE_FILE = "/radar.png";
const char* SINGAPORE_MAP_FILE = "/base-853-final.png";

PNG png;


void* myOpen(const char *filename, int32_t *size) {
  File *f = new File(SPIFFS.open(filename, "r"));
  *size = f->size();
  return (void *)f;
}

void myClose(void *handle) {
  File *f = (File *)handle;
  f->close();
  delete f;
}

int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
  File *f = (File *)handle->fHandle;
  return f->read(buffer, length);
}

int32_t mySeek(PNGFILE *handle, int32_t position) {
  File *f = (File *)handle->fHandle;
  return f->seek(position);
}

uint8_t getHue(uint16_t color) {
    // 1. Extract raw components
    uint8_t r = (color >> 11) & 0x1F;      // 5-bit (0-31)
    uint8_t g = (color >> 5) & 0x3F;       // 6-bit (0-63)
    uint8_t b = color & 0x1F;              // 5-bit (0-31)

    // 2. Normalize Green to 5-bit ONLY for min/max comparison
    uint8_t gn = g >> 1; 
    
    uint8_t maxC = (r > gn) ? (r > b ? r : b) : (gn > b ? gn : b);
    uint8_t minC = (r < gn) ? (r < b ? r : b) : (gn < b ? gn : b);
    uint8_t delta = maxC - minC;

    // 3. Filter noise/grayscale
    // If delta is small, the color is too 'grey' to have a reliable hue
    if (delta < 10) return 255; 

    int16_t hue;
    
    // 4. Precision Hue Math
    // We use the full 6-bit 'g' here. 
    // We bit-shift 'r' and 'b' (<< 1) to match green's 6-bit scale.
    if (maxC == r) {
        hue = (int16_t)(g - (b << 1)) * 32 / (delta << 1); 
    } else if (maxC == gn) {
        hue = 64 + (int16_t)((b << 1) - (r << 1)) * 32 / (delta << 1);
    } else {
        hue = 128 + (int16_t)((r << 1) - g) * 32 / (delta << 1);
    }

    // 5. Wrap around the 0-191 circle
    if (hue < 0) hue += 192;
    return (uint8_t)(hue % 192);
}

int pngDrawCallback(PNGDRAW *pDraw) {
  // Retrieve the buffer we passed in png.decode()
  uint16_t *pPixels = (uint16_t *)pDraw->pUser;

  int png_width = pDraw->iWidth;
  int png_height = png.getHeight(); // Use pDraw directly
  int display_width = display.width();
  int display_height = display.height();

  // Pre-calculate scales (Ideally these would be in the pUser struct to save CPU)
  uint32_t scale_fp = (uint32_t)(((uint64_t)display_width << 16) / png_width);
  uint32_t scale_y_fp = (uint32_t)(((uint64_t)display_height << 16) / png_height);
  if (scale_y_fp < scale_fp) scale_fp = scale_y_fp;

  int scaled_height = (png_height * scale_fp) >> 16;
  int y_offset = (display_height - scaled_height) / 2;

  int y_start = ((pDraw->y * scale_fp) >> 16) + y_offset;
  int y_end   = (((pDraw->y + 1) * scale_fp) >> 16) + y_offset;
  int h = y_end - y_start;
  if (h <= 0) h = 1; 

  // Decode line into the buffer passed via pUser
  png.getLineAsRGB565(pDraw, pPixels, PNG_RGB565_LITTLE_ENDIAN, 0xFFFFFF);
  
  uint16_t drawColor = GxEPD_BLACK;
  
  for (int x = 0; x < png_width; x++) {
    uint16_t color = pPixels[x];

    if (color != 0xFFFF ) { 
      int x_start = (x * scale_fp) >> 16;
      int x_end   = ((x + 1) * scale_fp) >> 16;
      int w = x_end - x_start;
      if (w <= 0) w = 1;
      bool upscale = (w > 1 && h > 1);
      if (upscale) {
          display.fillRect(x_start, y_start, w, h, drawColor);
      } else {
          display.drawPixel(x_start, y_start, drawColor);
      }
    }
  }
 
  return 1;
}

// Pre-calculated tables (Global or in your class)
uint8_t hueThresholdTable[192];
uint16_t hueColorTable[192];

// Run this ONCE in setup()
void precomputeHueTables() {
    // The total distance from Cyan (96) back to Magenta (174) 
    // through Red (0) is approximately 114 units.
    const float maxSpan = 114.0f; 

    // This value (0.0 to 1.0) determines the maximum "fill" 
    // 0.90 means 90% ink, 10% white dots.
    const float maxDensityCap = 0.92f;    

    for (int h = 0; h < 192; h++) {
        // 1. REVERSE HUE DIRECTION
        // (StartValue - CurrentValue) wraps correctly to map 
        // Cyan(96)->0, Green(64)->32, Yellow(32)->64, Red(0)->96, Magenta(174)->114
        int sH = (96 - h + 192) % 192; 

        // 2. CALCULATE INTENSITY
        float rainInt = sH / maxSpan;
        
        // CROP: If sH is outside our valid rain span (e.g. into the Blue hues), kill it.
        // This prevents the gap between Magenta(174) and Cyan(96) from drawing garbage.
        if (sH > 128) rainInt = 0; 
        
        rainInt = constrain(rainInt, 0.0f, 1.0f);

        // 3. DENSITY (Blue Noise Threshold)
        // We want the density to hit 100% (Solid) relatively early.
        // A multiplier of 1.4 ensures Red (rainInt 0.84) and Orange are solid.
        //float density = rainInt * 1.4f;
        float gamma_power = 0.85f; // Adjust this to control how quickly density ramps up. >1 means more aggressive early saturation.
        float density = pow(rainInt, gamma_power) * maxDensityCap;
        hueThresholdTable[h] = (uint8_t)(constrain(density, 0.0f, 1.0f) * 255);

        // 4. COLOR SELECTION
        // Magenta cores (rainInt > 0.9) become RED on the E-Ink display.
        // All other rain levels become BLACK.
        hueColorTable[h] = (rainInt > 0.9f) ? GxEPD_RED : GxEPD_BLACK;
        

        // 5. NOISE FLOOR
        // The table shows Hue 94-97 is the "mist". We ignore the first 2% 
        // of the intensity scale to keep the display clear of sensor noise.
        if (rainInt < 0.005f) hueThresholdTable[h] = 0;
    }
}

int pngBlueNoiseCallback(PNGDRAW *pDraw) {
    uint16_t *pPixels = (uint16_t *)pDraw->pUser;

    int png_width = pDraw->iWidth;
    int png_height = png.getHeight();
    int display_width = display.width();
    int display_height = display.height();

    // 1. Fixed-Point Scaling Logic
    uint32_t scale_fp = (uint32_t)(((uint64_t)display_width << 16) / png_width);
    uint32_t scale_y_fp = (uint32_t)(((uint64_t)display_height << 16) / png_height);
    if (scale_y_fp < scale_fp) scale_fp = scale_y_fp;

    int scaled_height = (png_height * scale_fp) >> 16;
    int y_offset = (display_height - scaled_height) / 2;

    int y_start = ((pDraw->y * scale_fp) >> 16) + y_offset;
    int y_end   = (((pDraw->y + 1) * scale_fp) >> 16) + y_offset;
    int h = y_end - y_start;
    if (h <= 0) h = 1;

    // 2. Decode the line
    png.getLineAsRGB565(pDraw, pPixels, PNG_RGB565_LITTLE_ENDIAN, 0xFFFFFF);
    
    for (int x = 0; x < png_width; x++) {
        uint16_t color = pPixels[x];
        if (color == 0xFFFF || color == 0x0000) continue; // Skip White/Black background

        int hue = getHue(color);
        if (hue == 255) continue; 

        // 3. Fast Lookup for Threshold and Color
        uint8_t drawThreshold = hueThresholdTable[hue];
        if (drawThreshold == 0) continue; 
        uint16_t drawColor = hueColorTable[hue];

        // 4. Per-Pixel Scaled Drawing
        int x_start = (x * scale_fp) >> 16;
        int x_end   = ((x + 1) * scale_fp) >> 16;
        int w = x_end - x_start;
        if (w <= 0) w = 1;

        // Loop through every physical pixel in the "upscaled" block
        for (int dy = 0; dy < h; dy++) {
            for (int dx = 0; dx < w; dx++) {
                int curX = x_start + dx;
                int curY = y_start + dy;

                uint8_t noise = bluenoise256[(curY % 256) * 256 + (curX % 256)];

                // Only draw if the individual pixel passes the noise test
                if (noise <= drawThreshold) {
                    display.drawPixel(curX, curY, drawColor);
                }
            }
        }
    }
    return 1;
}

uint32_t rainPixelCount = 0;

int pngCountRainCallback(PNGDRAW *pDraw) {
    uint16_t *pPixels = (uint16_t *)pDraw->pUser;
    png.getLineAsRGB565(pDraw, pPixels, PNG_RGB565_LITTLE_ENDIAN, 0xFFFFFF);

    for (int x = 0; x < pDraw->iWidth; x++) {
        // Skip background colors (White/Black)
        if (pPixels[x] == 0xFFFF || pPixels[x] == 0x0000) continue; 

        // Any valid hue is rain
        if (getHue(pPixels[x]) != 255) {
            rainPixelCount++;
        }
    }
    return 1;
}

int getRainCoverPercentage() {
    rainPixelCount = 0;
    int percentage = 0;

    // Use a fixed buffer size based on your max width
    uint16_t* lineBuffer = (uint16_t*)malloc(480 * sizeof(uint16_t));
    if (!lineBuffer) return 0;

    if (png.open(RADAR_IMAGE_FILE, myOpen, myClose, myRead, mySeek, pngCountRainCallback) == PNG_SUCCESS) {
        uint32_t width = png.getWidth();
        uint32_t height = png.getHeight();
        uint32_t totalPixels = width * height;

        png.decode(lineBuffer, 0); 
        png.close();

        if (totalPixels > 0) {
            // (Pixels * 100) / total = Whole number percentage
            // 230,400 * 100 = 23 million (Safe for uint32_t)
            percentage = (int)((rainPixelCount * 100) / totalPixels);
        }
    }
    Serial.println("Rain cover percentage: " + String(percentage) + "%");
    free(lineBuffer);
    return percentage;
}


String getRadarUrl(int delay) {
  // Create a copy of globalTimeInfo
  struct tm targetTime = globalTimeInfo;
  
  // Set to nearest previous 5-minute interval
  targetTime.tm_min = (targetTime.tm_min / 5) * 5;
  targetTime.tm_sec = 0;  // Set seconds to 0 for consistency
  
  // Convert to time_t and subtract 15 minutes (15 * 60 seconds)
  time_t t = mktime(&targetTime);
  t -= (delay * 60);
  
  // Convert back to struct tm using localtime (works reliably on ESP32)
  struct tm *adjustedTime = localtime(&t);
  
  // Format the timestamp
  char ts[17];
  snprintf(ts, sizeof(ts), "%04d%02d%02d%02d%02d0000",
    adjustedTime->tm_year + 1900,
    adjustedTime->tm_mon + 1,
    adjustedTime->tm_mday,
    adjustedTime->tm_hour,
    adjustedTime->tm_min);

  //return "https://www.nea.gov.sg/docs/default-source/rain-area/dpsri_70km_2026021416050000dBR.dpsri.png";
  //return "https://www.nea.gov.sg/docs/default-source/rain-area/dpsri_70km_2026021416450000dBR.dpsri.png";
  return String("https://www.nea.gov.sg/docs/default-source/rain-area/dpsri_70km_") + ts + "dBR.dpsri.png";
}

bool fetchRadarImage(const String &url) {
  HTTPClient http;
  http.begin(url);
  int r = http.GET();
  if (r == HTTP_CODE_OK) {
    size_t len = http.getSize();
    WiFiClient *stream = http.getStreamPtr();

    File f = SPIFFS.open(RADAR_IMAGE_FILE, FILE_WRITE);
    while (len && stream->available()) {
      uint8_t buf[128];
      size_t sz = stream->readBytes(buf, sizeof(buf));
      f.write(buf, sz);
      len -= sz;
    }
    f.close();
    http.end();
    return true;
  }
  http.end();
  return false;
}

bool isUrlAlreadyDownloaded(const String &url) {
  if (!SPIFFS.exists(DOWNLOADED_URL_FILE)) {
    return false;
  }
  
  File f = SPIFFS.open(DOWNLOADED_URL_FILE, FILE_READ);
  if (!f) {
    return false;
  }
  
  // Read the single URL from file
  String savedUrl = f.readString();
  f.close();
  
  savedUrl.trim();  // Remove any whitespace/newlines
  
  bool isSame = (savedUrl == url);
  
  
  if (isSame) {
    Serial.print("URL already downloaded: ");
    Serial.println(url);
  }
  
  
  return isSame;
}

void saveDownloadedUrl(const String &url) {
  File f = SPIFFS.open(DOWNLOADED_URL_FILE, FILE_WRITE);
  if (f) {
    f.print(url);
    f.close();
    Serial.print("Saved URL: ");
    Serial.println(url);
  }
}




bool displaySingaporeMapWithRadarOverlay() {
  // Allocate a line buffer for PNG decoding. This is reused for both the map and radar layers.
  // The size is based on the maximum expected width of the PNG. Adjust if you use wider images.
  uint16_t* localBuffer = (uint16_t*)malloc(400 * sizeof(uint16_t));
  if (!localBuffer) return false;

  display.firstPage();
  
  do {
    // DRAW MAP
    if (png.open(SINGAPORE_MAP_FILE, myOpen, myClose, myRead, mySeek, pngDrawCallback) == PNG_SUCCESS) {
      png.decode(localBuffer, 0);
      png.close();
    }

    // DRAW RADAR
    if (png.open(RADAR_IMAGE_FILE, myOpen, myClose, myRead, mySeek, pngBlueNoiseCallback) == PNG_SUCCESS) {
      png.decode(localBuffer, 0); 
      png.close();
    }

    displayBatteryLevel(); // Overlay battery level on top of the display

  } while (display.nextPage());

  free(localBuffer); // Free memory so other parts of the app can use it
  return true;
}
