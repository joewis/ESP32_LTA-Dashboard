# Copilot Instructions for ESP32 Bus Arrivals Display Project

## Project Overview
This project is an ESP32-based e-paper display system for showing Singapore bus arrivals, air quality, and radar map data. It uses the Arduino framework and PlatformIO for builds. The codebase is modular, with clear separation between display logic, data fetching, and configuration management.

## Key Components
- **src/main.cpp**: Entry point, orchestrates WiFi, NTP, display, and main loop.
- **src/DisplayInstance.cpp/h**: Instantiates and configures the e-paper display. Adjust pin assignments here for hardware changes.
- **src/configManager.cpp/h**: Handles loading, saving, and fetching configuration and data files (bus stops, destinations, time slots) from SPIFFS or GitHub.
- **src/busArrivalsService.cpp/h**: Fetches and processes bus arrival data from the LTA API. Uses global JSON documents for data sharing.
- **src/busArrivalsDisplay.cpp/h**: Renders bus arrival and destination data to the display using U8g2 fonts and GxEPD2 graphics.
- **src/radarMap.cpp/h**: Handles radar and map image processing, including PNG decoding and drawing.

## Data Flow
- Configuration and static data are loaded from SPIFFS or fetched from GitHub (see `configManager`).
- Real-time data (bus arrivals, air quality) is fetched via HTTP APIs.
- Data is stored in global `JsonDocument` objects for cross-module access.
- Display rendering is handled in paged mode to save RAM (see `DisplayInstance.h`).

## Build & Flash
- **Build and upload:**
  ```sh
  platformio run --target upload
  ```
- **Serial monitor:**
  ```sh
  platformio device monitor
  ```
- **Configuration:**
  Edit `platformio.ini` for board, partition, and library settings.

## Project Conventions
- Use global `JsonDocument` objects for shared state (see `configManager.h`).
- Display buffer height is tuned for RAM usage (`MAX_DISPLAY_BUFFER_HEIGHT` in `DisplayInstance.h`).
- All display code uses the `display` and `u8g2Fonts` singletons.
- API keys and WiFi credentials are hardcoded in `main.cpp` (consider secrets management for production).
- Data files are referenced by both URL and local SPIFFS path in `ConfigFile` structs.

## Integration Points
- **LTA Bus Arrivals API**: See `busArrivalsService.cpp` for endpoint and key usage.
- **GitHub-hosted config/data**: URLs in `configManager.cpp`.
- **E-paper display**: GxEPD2 and U8g2 libraries, see `DisplayInstance.*`.

## Examples
- To add a new data source, create a new module and add its data to a global `JsonDocument`.
- To change display layout, edit rendering functions in `busArrivalsDisplay.cpp` and `main.cpp`.

## References
- [platformio.ini](../platformio.ini): Build config and dependencies
- [src/](../src/): All source code modules

---
If any section is unclear or missing, please provide feedback for further refinement.
