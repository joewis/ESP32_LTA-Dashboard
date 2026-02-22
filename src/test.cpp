#include "DisplayInstance.h"
#include "blue_noise.h"

void testAllPatternsLabeled() {
    int blockSize = 32; // Increased size so labels are legible
    int w = display.width();  
    int h = display.height(); 
    int cols = w / blockSize;

    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    // Use a small font if available, otherwise default
    display.setTextSize(1); 

    for (int y = 0; y < h - blockSize; y += blockSize) {
        for (int x = 0; x < w - blockSize; x += blockSize) {
            
            int blockIdx = (x / blockSize) + (y / blockSize) * cols;
            int pattern = blockIdx % 24;

            // 1. Draw the Pattern
            for (int py = 0; py < blockSize; py++) {
                for (int px = 0; px < blockSize; px++) {
                    uint16_t c = GxEPD_WHITE;
                    int pSum = px + py;

                    switch (pattern) {
                        case 0:  c = GxEPD_WHITE; break;
                        case 1:  c = GxEPD_BLACK; break;
                        case 2:  c = GxEPD_RED;   break;
                        case 3:  c = ((px % 2 == 0) && (py % 2 == 0)) ? GxEPD_RED : GxEPD_WHITE; break; 
                        case 4:  c = (pSum % 2 == 0) ? GxEPD_RED : GxEPD_WHITE; break;
                        case 5:  c = ((px % 2 == 0) || (py % 2 == 0)) ? GxEPD_RED : GxEPD_WHITE; break;
                        case 6:  c = ((px % 2 == 0) && (py % 2 == 0)) ? GxEPD_BLACK : GxEPD_WHITE; break;
                        case 7:  c = (pSum % 2 == 0) ? GxEPD_BLACK : GxEPD_WHITE; break;
                        case 8:  c = ((px % 2 == 0) || (py % 2 == 0)) ? GxEPD_BLACK : GxEPD_WHITE; break;
                        case 9:  c = (pSum % 2 == 0) ? GxEPD_RED : GxEPD_BLACK; break;
                        case 10: c = (px % 3 == 0) ? GxEPD_RED : GxEPD_BLACK; break;
                        case 11: c = (px % 2 == 0 && py % 2 == 0) ? GxEPD_RED : GxEPD_BLACK; break;
                        case 12: c = (py < 4) ? GxEPD_BLACK : GxEPD_WHITE; break; 
                        case 13: c = (px < 4) ? GxEPD_RED : GxEPD_WHITE; break;
                        case 14: c = (px == py) ? GxEPD_BLACK : GxEPD_WHITE; break;
                        case 15: c = (px == (blockSize - py - 1)) ? GxEPD_RED : GxEPD_WHITE; break;
                        case 16: c = ((px ^ py) & 4) ? GxEPD_BLACK : GxEPD_WHITE; break;
                        case 17: c = ((px * py) % 5 == 0) ? GxEPD_RED : GxEPD_WHITE; break;
                        case 18: c = ((px * px + py * py) % 10 < 5) ? GxEPD_BLACK : GxEPD_WHITE; break;
                        case 19: c = (esp_random() % 100 < 20) ? GxEPD_RED : GxEPD_WHITE; break;
                        case 20: c = (esp_random() % 100 < 20) ? GxEPD_BLACK : GxEPD_WHITE; break;
                        case 21: c = (px % 3 == 0) ? GxEPD_RED : (px % 3 == 1 ? GxEPD_BLACK : GxEPD_WHITE); break;
                        case 22: c = (pSum % 3 == 0) ? GxEPD_RED : (pSum % 3 == 1 ? GxEPD_BLACK : GxEPD_WHITE); break;
                        case 23: c = (px % 4 == 0 || py % 4 == 0) ? GxEPD_RED : GxEPD_BLACK; break;
                    }
                    display.drawPixel(x + px, y + py, c);
                }
            }

            // 2. Overlay the Label Box
            // We draw a small white rectangle in the corner of each block
            display.fillRect(x + 2, y + 2, 14, 10, GxEPD_WHITE);
            display.setCursor(x + 3, y + 3);
            display.print(pattern);
        }
    }
    display.display();
}

void drawOrderedColorPicker() {
    int w = display.width();
    int h = display.height();

    // Standard 8x8 Bayer Threshold Matrix
    const int bayer[8][8] = {
        { 0, 32,  8, 40,  2, 34, 10, 42},
        {48, 16, 56, 24, 50, 18, 58, 26},
        {12, 44,  4, 36, 14, 46,  6, 38},
        {60, 28, 52, 20, 62, 30, 54, 22},
        { 3, 35, 11, 43,  1, 33,  9, 41},
        {51, 19, 59, 27, 49, 17, 57, 25},
        {15, 47,  7, 39, 13, 45,  5, 37},
        {63, 31, 55, 23, 61, 29, 53, 21}
    };

    display.fillScreen(GxEPD_WHITE);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // Map coordinates to 0-63 intensity (matching our 8x8 matrix)
            int blackIntensity = (y * 64) / h;
            int redIntensity   = (x * 64) / w;

            // Get the threshold for the current pixel position
            int threshold = bayer[y % 8][x % 8];

            // 1. Draw to the Black Plane
            if (threshold < blackIntensity) {
                display.drawPixel(x, y, GxEPD_BLACK);
            }

            // 2. Draw to the Red Plane
            // Since they are separate planes, we can draw on top of black
            if (threshold < redIntensity) {
                display.drawPixel(x, y, GxEPD_RED);
            }
        }
    }
    

    display.display();
}

void drawBlueNoisePicker() {
    int w = display.width();
    int h = display.height();

    display.fillScreen(GxEPD_WHITE);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // Map X and Y to our 64x64 mask (tiling)
            int maskIndex = (y % 64) * 64 + (x % 64);
            uint8_t threshold = blueNoise64[maskIndex];

            // Calculate target intensity (0-255)
            // Y-axis: 0 at top, 255 at bottom (Black)
            // X-axis: 0 at left, 255 at right (Red)
            int blackTarget = (y * 255) / h;
            int redTarget   = (x * 255) / w;

            // 1. Draw to Black Plane
            if (threshold < blackTarget) {
                display.drawPixel(x, y, GxEPD_BLACK);
            }

            // 2. Draw to Red Plane
            // Note: Since threshold is a unified map, 
            // the red and black pixels will align perfectly.
            if (threshold < redTarget) {
                display.drawPixel(x, y, GxEPD_RED);
            }
        }
    }
    display.display();
}


void testColorCapabilities() {
    // Replace with your actual display width/height variables
    int w = display.width();
    int h = display.height();
    int blockSize = 6;
    
    display.fillScreen(GxEPD_WHITE);

    for (int y = 0; y < h; y += blockSize) {
        for (int x = 0; x < w; x += blockSize) {
            
            // Cycle through 7 patterns based on block position
            int pattern = ((x / blockSize) + (y / blockSize)) % 7;
            uint16_t drawColor;

            for (int py = 0; py < blockSize; py++) {
                for (int px = 0; px < blockSize; px++) {
                    
                    switch (pattern) {
                        case 0: drawColor = GxEPD_WHITE; break; // Solid White
                        case 1: drawColor = GxEPD_BLACK; break; // Solid Black
                        case 2: drawColor = GxEPD_RED;   break; // Solid Red
                        
                        case 3: // Checkerboard Black/White (Simulated Grey)
                            drawColor = ((px + py) % 2 == 0) ? GxEPD_BLACK : GxEPD_WHITE;
                            break;
                            
                        case 4: // Checkerboard Red/White (Simulated Pink)
                            drawColor = ((px + py) % 2 == 0) ? GxEPD_RED : GxEPD_WHITE;
                            break;
                            
                        case 5: // Checkerboard Red/Black (Dark Red/Maroon)
                            drawColor = ((px + py) % 2 == 0) ? GxEPD_RED : GxEPD_BLACK;
                            break;

                        case 6: // Vertical Stripes Red/Black/White
                            if (px % 3 == 0)      drawColor = GxEPD_RED;
                            else if (px % 3 == 1) drawColor = GxEPD_BLACK;
                            else                  drawColor = GxEPD_WHITE;
                            break;
                    }
                    
                    display.drawPixel(x + px, y + py, drawColor);
                }
            }
        }
    }
    display.display();
}

void testAllPatterns() {
    int blockSize = 12; // Larger blocks to see the texture better
    int w = display.width();
    int h = display.height();

    for (int y = 0; y < h; y += blockSize) {
        for (int x = 0; x < w; x += blockSize) {
            
            // 16 patterns loop
            int pattern = ((x / blockSize) + (y / blockSize) * (w / blockSize)) % 16;

            for (int py = 0; py < blockSize; py++) {
                for (int px = 0; px < blockSize; px++) {
                    uint16_t c;
                    int pSum = px + py;

                    switch (pattern) {
                        // --- SOLIDS ---
                        case 0: c = GxEPD_WHITE; break;
                        case 1: c = GxEPD_BLACK; break;
                        case 2: c = GxEPD_RED;   break;

                        // --- 50% DITHERING (Checkerboards) ---
                        case 3: c = (pSum % 2 == 0) ? GxEPD_BLACK : GxEPD_WHITE; break;
                        case 4: c = (pSum % 2 == 0) ? GxEPD_RED : GxEPD_WHITE;   break;
                        case 5: c = (pSum % 2 == 0) ? GxEPD_RED : GxEPD_BLACK;   break;

                        // --- 25% DITHERING (Sparse Dots) ---
                        case 6: c = (px % 2 == 0 && py % 2 == 0) ? GxEPD_BLACK : GxEPD_WHITE; break;
                        case 7: c = (px % 2 == 0 && py % 2 == 0) ? GxEPD_RED : GxEPD_WHITE;   break;

                        // --- STRIPES (Interference test) ---
                        case 8:  c = (px % 2 == 0) ? GxEPD_BLACK : GxEPD_WHITE; break; // V-Black
                        case 9:  c = (py % 2 == 0) ? GxEPD_RED : GxEPD_WHITE;   break; // H-Red
                        case 10: c = (px % 3 == 0) ? GxEPD_RED : GxEPD_BLACK;   break; // V-Red/Black

                        // --- COMPLEX TEXTURES ---
                        case 11: c = ((px * py) % 3 == 0) ? GxEPD_BLACK : GxEPD_WHITE; break; // Crosshatch
                        case 12: c = ((px ^ py) % 4 == 0) ? GxEPD_RED : GxEPD_WHITE;   break; // Fractal-ish
                        case 13: c = (px == py || px == (blockSize - py)) ? GxEPD_RED : GxEPD_BLACK; break; // X-Pattern
                        case 14: c = (px % 2 == py % 2) ? GxEPD_RED : (px % 2 == 0 ? GxEPD_BLACK : GxEPD_WHITE); break; // 3-Color Mix
                        case 15: c = (esp_random() % 2 == 0) ? GxEPD_RED : GxEPD_BLACK; break; // Random Noise
                    }
                    display.drawPixel(x + px, y + py, c);
                }
            }
        }
    }
    display.display();
}