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

                uint8_t noise = pgm_read_byte(&blueNoise64[(y % 64) * 64 + (x % 64)]);
                
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

void displayMandelbrot() {
    uint8_t ditherTable[33];
    for (int i = 0; i <= 32; i++) {
        ditherTable[i] = (uint8_t)((i * i * 255) / (1024)); // Scaled for 32 iterations
    }

    int w = display.width();
    int h = display.height();

    // Randomize position and zoom for different fractal views
    // Instead of completely random values, we'll use ranges that ensure good coverage
    float aspectRatio = (float)w / (float)h;
    
    // Choose from predefined interesting regions of the Mandelbrot set
    int regionChoice = random(0, 5);
    float centerX, centerY, zoomFactor;
    
    switch(regionChoice) {
        case 0: // Full view of the main set
            centerX = -0.5f;
            centerY = 0.0f;
            zoomFactor = 2.5f; // This will make the main set fill the screen appropriately
            break;
        case 1: // Seahorse valley
            centerX = -0.75f;
            centerY = 0.1f;
            zoomFactor = 100.0f;
            break;
        case 2: // Lightning
            centerX = -1.775f;
            centerY = 0.0f;
            zoomFactor = 200.0f;
            break;
        case 3: // Mini-Mandelbrot
            centerX = -1.25066f;
            centerY = 0.02012f;
            zoomFactor = 500.0f;
            break;
        case 4: // Spiral area
            centerX = -0.16f;
            centerY = 1.04f;
            zoomFactor = 150.0f;
            break;
        default:
            centerX = -0.5f;
            centerY = 0.0f;
            zoomFactor = 2.5f;
            break;
    }
    
    // Apply some randomization around the chosen region
    float randOffsetX = (float)random(-50, 50) / 100.0f / zoomFactor;
    float randOffsetY = (float)random(-50, 50) / 100.0f / zoomFactor;
    float randZoomAdjust = (float)random(80, 120) / 100.0f; // 0.8 to 1.2 multiplier
    
    centerX += randOffsetX;
    centerY += randOffsetY;
    zoomFactor *= randZoomAdjust;

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        for (int y = 0; y < h; y++) {
            float cIm = centerY + (y - h / 2.0f) / (zoomFactor * h / 4.0f);
            for (int x = 0; x < w; x++) {
                float cRe = centerX + (x - w / 2.0f) / (zoomFactor * w / 4.0f * aspectRatio);
                
                float zRe = 0.0f;
                float zIm = 0.0f;
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
                    // Inside the Mandelbrot set - use red with dithering for internal detail
                    float mag = sqrt(zRe * zRe + zIm * zIm);
                    uint8_t innerThreshold = (uint8_t)(mag * 120);
                    
                    if (noise > innerThreshold) {
                        display.drawPixel(x, y, GxEPD_RED);
                    } else {
                        display.drawPixel(x, y, GxEPD_BLACK);
                    }
                } else if (i > 3) {
                    // Outside the set - use dithering for grayscale effect
                    if (noise < ditherTable[i]) {
                        display.drawPixel(x, y, GxEPD_BLACK);
                    }
                }
            }
            if (y % 20 == 0) yield();
        }
    } while (display.nextPage());
}