#include <Arduino.h>
#include <GxEPD2_3C.h>
#include "DisplayInstance.h"
#include "blue_noise.h"

void displayJuliaSet() {
    uint8_t ditherTable[33];
    for (int i = 0; i <= 32; i++) {
        ditherTable[i] = (uint8_t)((i * i * 255) / (1024)); // Scaled for 32 iterations
    }

    int w = display.width();
    int h = display.height();

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

        for (int y = 0; y < h; y++) {
            float startIm = (y - h / 2.0f) / (0.5f * 1.2f * h);
            for (int x = 0; x < w; x++) {
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

                uint8_t noise = bluenoise256[(y % 256) * 256 + (x % 256)];
                
                if (i == 32) {
                    // --- THE FIX: Dither the Red interior ---
                    // Use the final magnitude to create internal detail
                    float mag = sqrt(zRe * zRe + zIm * zIm);
                    uint8_t innerThreshold = (uint8_t)(mag * 120); // Adjust 120 for "vein" density
                    
                    if (noise > innerThreshold) {
                        display.drawPixel(x, y, GxEPD_RED);
                    } else {
                        // This creates white/black veins inside the red blob
                        display.drawPixel(x, y, GxEPD_BLACK);
                    }
                } else if (i > 3) {
                    // Exterior black dithering
                    if (noise < ditherTable[i]) {
                        display.drawPixel(x, y, GxEPD_BLACK);
                    }
                }
            }
            if (y % 20 == 0) yield();
        }
    } while (display.nextPage());
}

struct MandelSpot {
    float x;
    float y;
    float z;
    const char* name;
};

// 16 High-detail spots across the Mandelbrot Set
const MandelSpot midgetLibrary[16] = {
    {-0.5f, 0.0f, 2.2f, "The Big Picture"},
    {-0.7453f, 0.1127f, 650.0f, "Seahorse Valley"},
    {-0.1607f, 1.0375f, 1200.0f, "Triple Spiral"},
    {0.2817f, 0.5318f, 500.0f, "Elephant Valley"},
    {-1.2506f, 0.0201f, 1500.0f, "Mini Mandelbrot"},
    {-0.748f, 0.1f, 2000.0f, "The Seahorse Eye"},
    {-1.775f, 0.0f, 300.0f, "Lightning Bolt"},
    {-0.8115f, 0.2014f, 1800.0f, "Deep Spiral"},
    {-0.1011f, 0.9563f, 500.0f, "Medusa Tentacles"},
    {-1.476f, 0.0f, 4500.0f, "The Satellite"},
    {-0.374f, 0.659f, 1200.0f, "Starfish Branch"},
    {-1.2505f, 0.0471f, 800.0f, "Scepter Valley"},
    {-0.743f, 0.131f, 3500.0f, "Deep Forest"},
    {0.273f, 0.007f, 500.0f, "The Cusp"},
    {-0.1607f, 1.0375f, 2500.0f, "Quad-Spiral Zoom"},
    {-1.94f, 0.0f, 1200.0f, "Antenna Tip"}
};

void displayMandelbrot() {
  const int maxIterations = 256; 
  int w = display.width();
  int h = display.height();
  float aspectRatio = (float)w / (float)h;

  // 1. Coordinates (Picking a guaranteed high-detail spot)
  //float centerX = -0.7453f; 
  //float centerY = 0.1127f;
  //float zoomFactor = 650.0f;

  int choice = random(0, 16);
  MandelSpot spot = midgetLibrary[choice];

  float centerX = spot.x;
  float centerY = spot.y;
  float zoomFactor = spot.z;

  //centerX += (float)random(-100, 100) / (zoomFactor * 5000.0f);
  //centerY += (float)random(-100, 100) / (zoomFactor * 5000.0f);
  //zoomFactor *= (float)random(10, 20) / 10.0f; // Add up to ±50% random zoom for variety

  Serial.printf("Rendering %s at Zoom %.1f\n", spot.name, zoomFactor);

  display.setFullWindow();
  display.firstPage();
  int currentPage = 0;

  do {
    display.fillScreen(GxEPD_WHITE);

    int pageYStart = display.pageHeight()  * currentPage;

    //for (int y = 0; y < h; y++) {
    for (int y = pageYStart; y < pageYStart + display.pageHeight() ; y++) {
        float cIm = centerY + (y - h / 2.0f) * (4.0f / (zoomFactor * h));
      for (int x = 0; x < w; x++) {
        float cRe = centerX + (x - w / 2.0f) * (4.0f / (zoomFactor * w)) * aspectRatio;
        float zRe = 0.0f, zIm = 0.0f;
        int i;

        for (i = 0; i < maxIterations; i++) {
            float r2 = zRe * zRe;
            float i2 = zIm * zIm;
            if (r2 + i2 > 4.0f) break;
            zIm = 2.0f * zRe * zIm + cIm;
            zRe = r2 - i2 + cRe;
        }

        uint8_t noise = bluenoise256[(y % 256) * 256 + (x % 256)];

        // --- LAYERED COLORING LOGIC ---
        if (i == maxIterations) {
            // THE CORE: Create a "Glow" from the boundary inward
            float mag = sqrt(zRe * zRe + zIm * zIm);
            
            // Normalize mag (approx 0.0 to 2.0) to a 0.0-1.0 range
            float normalizedMag = mag / 2.0f;
            
            // --- THE GLOW CURVE ---
            // pow(x, 3.0) makes the red density drop off sharply as you move 
            // away from the edge. Use 2.0 for a softer, deeper glow.
            float glowIntensity = pow(normalizedMag, 0.8f);
            
            // We cap the max density at 160 to keep the core primarily black
            uint8_t innerThreshold = (uint8_t)(glowIntensity * 160);
            
            if (noise < innerThreshold) {
                display.drawPixel(x, y, GxEPD_RED);
            } else {
                display.drawPixel(x, y, GxEPD_BLACK);
            }
        } 
        else if (i > 180) {
            // INNER EDGE: Transition layer (Black/Red mix)
            if (noise < 120) display.drawPixel(x, y, GxEPD_BLACK);
            else display.drawPixel(x, y, GxEPD_RED);
        }
        else if (i > 100) {
            // CORONA: Solid Red pop
            display.drawPixel(x, y, GxEPD_RED);
        }
        else if (i > 10) {
            // HALO: Smooth bridge to white background
            float progress = (float)(i - 45) / (100 - 45); 
            uint8_t ditherThreshold = (uint8_t)(progress * progress * 255);
            if (noise < ditherThreshold) display.drawPixel(x, y, GxEPD_RED);
        }
      }
      //if (y % 15 == 0) yield();
    }
    currentPage++;
  } while (display.nextPage());
}


