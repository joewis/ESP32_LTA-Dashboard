#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "DisplayInstance.h"
#include "blue_noise.h"
#include "Utils.h"

extern String timestamp;
extern U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

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
    {0.2817f, 0.5318f, 100.0f, "Elephant Valley"},
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
    {-1.94f, 0.0f, 1200.0f, "Antenna Tip"},
    {-0.1560f, 1.0325f, 1800.0f, "Kiko's Rose"}
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

  int choice = random(0, 17);
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
            // mag stands for the distance from the escape point, which creates internal structure in the red core. Points just inside the edge will have a mag close to 2.0, while deeper points will approach 0.0. This allows us to create a density gradient for the red glow.
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
      if (y % 15 == 0) yield(); // Yield every 15 lines to keep WiFi responsive (adjust as needed)
    }

    displayBatteryLevel(timestamp); // Overlay battery level on top of the fractal

    // Label: show zoom spot name at bottom-left
    u8g2Fonts.setFont(u8g2_font_helvB08_tr);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setCursor(5, display.height() - 8);
    u8g2Fonts.printf("%s", spot.name);

    currentPage++;
  } while (display.nextPage());
}


struct BulbSpot {
    float camX, camY, camZ;
    float targetX, targetY, targetZ;
    const char* name;
};

const BulbSpot bulbLibrary[4] = {
    {0.0f, -2.5f, 0.0f, 0.0f, 0.0f, 0.0f, "The Full Bulb"},
    {0.5f, -0.8f, 0.5f, 0.2f, 0.2f, 0.2f, "Deep Canyon"},
    {0.0f, -1.5f, 0.8f, 0.0f, 0.0f, 0.1f, "Top-Down Fractal"}
};

// Distance Estimator for Mandelbulb Power 8
float getMandelbulbDE(float px, float py, float pz) {
    float x = px, y = py, z = pz;
    float dr = 1.0f;
    float r = 0.0f;
    const int iterations = 10; // Lower than 2D because it runs per Ray Step

    for (int i = 0; i < iterations; i++) {
        r = sqrtf(x*x + y*y + z*z);
        if (r > 2.0f) break;

        // Convert to spherical coordinates
        float theta = acosf(z / r);
        float phi = atan2f(y, x);
        float zr = powf(r, 7.0f); // Power - 1
        dr = zr * 8.0f * dr + 1.0f;

        // Scale and rotate
        float str = powf(r, 8.0f);
        theta *= 8.0f;
        phi *= 8.0f;

        // Convert back to cartesian
        x = str * sinf(theta) * cosf(phi) + px;
        y = str * sinf(theta) * sinf(phi) + py;
        z = str * cosf(theta) + pz;
    }
    return 0.5f * logf(r) * r / dr;
}

uint8_t gammaTable[256];

// Call this once in setup() or at the start of your display function
// Try gamma = 0.4 or 0.5 to 'lift' the dark areas
void precomputeGamma(float gamma, float maxDensity) {
    for (int i = 0; i < 256; i++) {
        float normalized = (float)i / 255.0f;
        float corrected = pow(normalized, gamma); 
        gammaTable[i] = (uint8_t)(corrected * 255.0f * maxDensity);
    }
}



void displayMandelbulb2() {
    // 1. Initialize the Gamma Table (Lift the shadows!)
    //choose gamma values based on the desired effect. A gamma of 0.5 will brighten the dark areas significantly, while 0.8 will be more subtle. The maxDensity parameter can be used to cap the brightness of the glow, preventing it from overpowering the core details.
    //to darken the image, use a gamma greater than 1.0 (e.g., 1.5 or 2.0) to compress the brightness range, making the dark areas darker and reducing the intensity of the glow.
    precomputeGamma(0.35f, 1.0f); 

    int w = display.width();
    int h = display.height();
    
    int choice = random(0, 4);
    BulbSpot spot = bulbLibrary[choice];
    Serial.printf("Ray Marching %s\n", spot.name);

    display.setFullWindow();
    display.firstPage();
    int currentPage = 0;

    do {
        display.fillScreen(GxEPD_WHITE);
        int pageYStart = display.pageHeight() * currentPage;

        for (int y = pageYStart; y < pageYStart + display.pageHeight(); y++) {
            float vy = (y - h / 2.0f) / h;
            for (int x = 0; x < w; x++) {
                float vx = (x - w / 2.0f) / w;
                
                float dx = vx, dy = 1.0f, dz = vy; 
                float dMag = sqrtf(dx*dx + dy*dy + dz*dz); //dmag is the length of the ray direction vector. Normalizing the ray direction is crucial for consistent ray marching, as it ensures that each step along the ray corresponds to a uniform distance in 3D space. Without normalization, rays that are longer would take larger steps, potentially skipping over details, while shorter rays would take smaller steps, increasing render time.
                dx /= dMag; dy /= dMag; dz /= dMag;

                float t = 0.0f;
                int steps = 0;
                float hitDE = 1000.0f;

                while (t < 4.0f && steps < 40) {
                    float curX = spot.camX + dx * t;
                    float curY = spot.camY + dy * t;
                    float curZ = spot.camZ + dz * t;
                    
                    hitDE = getMandelbulbDE(curX, curY, curZ); //hitDE stands for "hit Distance Estimator". It represents the estimated distance from the current point in space to the nearest surface of the Mandelbulb. The ray marching algorithm uses this value to determine how far to advance the ray in each step. If hitDE is very small (below a certain threshold), it indicates that the ray has likely hit the surface of the Mandelbulb, and the loop can terminate to render that pixel accordingly.
                    if (hitDE < 0.001f) break; 
                    t += hitDE;
                    steps++;
                }

                uint8_t noise = bluenoise256[(y % 256) * 256 + (x % 256)];

                // --- NEW GAMMA-CORRECTED COLORING ---
                if (hitDE < 0.001f) {
                    // SURFACE: We use the gamma table to brighten the crevices
                    float depthGlow = (float)steps / 40.0f;
                    int tableIdx = (int)(depthGlow * 255);
                    uint8_t threshold = gammaTable[tableIdx];

                    // Red acts as a highlight for the deepest areas
                    if (noise < threshold) {
                        if (steps > 30) display.drawPixel(x, y, GxEPD_RED);
                        else display.drawPixel(x, y, GxEPD_BLACK);
                    }
                } 
                else if (hitDE < 0.05f) {
                    // ATMOSPHERIC GLOW: Using Gamma to make the 'fog' softer
                    float proximity = (0.05f - hitDE) / 0.05f;
                    int tableIdx = (int)(proximity * 255);
                    uint8_t threshold = gammaTable[tableIdx];
                    
                    if (noise < (threshold / 2)) { // Fainter glow for near-misses
                        display.drawPixel(x, y, GxEPD_RED);
                    }
                }
            }
            if (y % 10 == 0) yield(); 
        }
        currentPage++;
    } while (display.nextPage());
}


void displayMandelbulb() {
    int w = display.width();
    int h = display.height();
    
    int choice = random(0, 3);
    BulbSpot spot = bulbLibrary[choice];
    Serial.printf("Ray Marching %s\n", spot.name);

    display.setFullWindow();
    display.firstPage();
    int currentPage = 0;

    do {
        display.fillScreen(GxEPD_WHITE);
        int pageYStart = display.pageHeight() * currentPage;

        for (int y = pageYStart; y < pageYStart + display.pageHeight(); y++) {
            // Very simple ray setup
            float vy = (y - h / 2.0f) / h;
            for (int x = 0; x < w; x++) {
                float vx = (x - w / 2.0f) / w;
                
                // Ray Direction
                float dx = vx, dy = 1.0f, dz = vy; 
                // Normalize direction
                float dMag = sqrtf(dx*dx + dy*dy + dz*dz);
                dx /= dMag; dy /= dMag; dz /= dMag;

                float t = 0.0f;     // Distance traveled along ray
                float minStep = 0.002f;
                int steps = 0;
                float hitDE = 1000.0f;

                // Ray Marching Loop
                while (t < 4.0f && steps < 40) {
                    float curX = spot.camX + dx * t;
                    float curY = spot.camY + dy * t;
                    float curZ = spot.camZ + dz * t;
                    
                    hitDE = getMandelbulbDE(curX, curY, curZ);
                    if (hitDE < 0.001f) break; // Surface hit
                    t += hitDE;
                    steps++;
                }

                uint8_t noise = bluenoise256[(y % 256) * 256 + (x % 256)];

                // --- 3D LAYERED COLORING ---
                if (hitDE < 0.001f) {
                    // SURFACE: We use 'steps' to create depth-shading
                    // High steps = hard to reach areas (crevices)
                    float depthGlow = (float)steps / 30.0f;
                    uint8_t threshold = (uint8_t)(depthGlow * 180);

                    if (noise < threshold) display.drawPixel(x, y, GxEPD_RED);
                    else display.drawPixel(x, y, GxEPD_BLACK);
                } 
                else if (hitDE < 0.05f) {
                    // ATMOSPHERIC GLOW: Near misses
                    float proximity = (0.05f - hitDE) / 0.05f;
                    uint8_t threshold = (uint8_t)(proximity * proximity * 200);
                    if (noise < threshold) display.drawPixel(x, y, GxEPD_RED);
                }
            }
            if (y % 10 == 0) yield(); // Keep the watchdog happy
        }
        currentPage++;
    } while (display.nextPage());
}
