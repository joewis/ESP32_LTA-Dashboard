# ESP32 Bus Dashboard

An ESP32-based bus dashboard system that displays real-time bus arrival information and radar maps on an e-paper display.

## Features

- Real-time bus arrival information display
- Weather radar map visualization
- E-paper display support (4.2 inch 3-color display)
- WiFi connectivity for data retrieval
- SPIFFS filesystem for local storage
- PNG image decoding support

## Hardware Requirements

- ESP32 development board
- 4.2 inch 3-color e-paper display (GxEPD2_420c)
- Appropriate power supply

## Software Dependencies

- Arduino framework
- PlatformIO build system
- Libraries:
  - ArduinoJson (v7.0.0+)
  - GxEPD2 (v1.5.0+)
  - PNGdec (v1.1.6+)
  - U8g2_for_Adafruit_GFX

## Project Structure

- `src/` - Source code files
  - `main.cpp` - Main application entry point
  - `configManager.*` - Configuration management
  - `busArrivalsService.*` - Bus arrival data handling
  - `busArrivalsDisplay.*` - Display logic for bus arrivals
  - `radarMap.*` - Radar map visualization
  - `DisplayInstance.*` - Display hardware abstraction
  - `blue_noise.h` - Blue noise texture for dithering
- `include/` - Header files
- `lib/` - Project-specific libraries
- `data/` - Static data files
- `test/` - Unit tests

## Configuration

The project uses a `secret.ini` file (not included in the repository) for sensitive configuration values. Create this file in your PlatformIO project root with appropriate build flags.

## Build Instructions

1. Install PlatformIO
2. Clone this repository
3. Run `pio run` to build the project
4. Connect your ESP32 board and run `pio run --target upload` to flash

## License

[Specify license here]
