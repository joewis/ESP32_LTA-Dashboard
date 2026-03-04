#pragma once
#include <Arduino.h>
#include "BatteryService.h"

class BatteryService {
public:
    void begin(int adcPin,
               float dividerFactor = 2.17f,
               float referenceMv = 3300.0f);

    float getVoltage();              // instant reading
    float getSmoothedVoltage();      // filtered reading
    int   getPercentage();           // 0–100%

    bool  isLow() const;             // e.g. < 20%
    bool  isCritical() const;        // e.g. < 10%

    void  setLowThreshold(int percent);
    void  setCriticalThreshold(int percent);

private:
    uint16_t readRaw() const;
    float rawToVoltage(uint16_t raw) const;
    int voltageToPercentage(float voltage) const;

    int   _pin;
    float _divider;
    float _referenceMv;

    float _filteredVoltage = 0.0f;
    int   _lowThreshold = 20;
    int   _criticalThreshold = 10;
};