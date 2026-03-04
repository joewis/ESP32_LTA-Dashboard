#include "BatteryService.h"


void BatteryService::begin(int adcPin,
                           float dividerFactor,
                           float referenceMv)
{
    _pin = adcPin;
    _divider = dividerFactor;
    _referenceMv = referenceMv;

    analogReadResolution(12); // ESP32 default
    _filteredVoltage = getVoltage(); // initialize filter
}

uint16_t BatteryService::readRaw() const
{
    return analogRead(_pin);
}

float BatteryService::rawToVoltage(uint16_t raw) const
{
    float mv = (_referenceMv * raw) / 4095.0f;
    float voltage = (mv / 1000.0f) * _divider;
    return voltage;
}

float BatteryService::getVoltage()
{
    return rawToVoltage(readRaw());
}

float BatteryService::getSmoothedVoltage()
{
    float newVoltage = getVoltage();

    // Exponential smoothing filter
    const float alpha = 0.1f;
    _filteredVoltage =
        (1.0f - alpha) * _filteredVoltage + alpha * newVoltage;

    return _filteredVoltage;
}

int BatteryService::getPercentage()
{
    float voltage = getSmoothedVoltage();
    return voltageToPercentage(voltage);
}

bool BatteryService::isLow() const
{
    return voltageToPercentage(_filteredVoltage) <= _lowThreshold;
}

bool BatteryService::isCritical() const
{
    return voltageToPercentage(_filteredVoltage) <= _criticalThreshold;
}

void BatteryService::setLowThreshold(int percent)
{
    _lowThreshold = percent;
}

void BatteryService::setCriticalThreshold(int percent)
{
    _criticalThreshold = percent;
}

// Nonlinear LiPo mapping (piecewise linear approximation)
int BatteryService::voltageToPercentage(float v) const
{
    if (v >= 4.20f) return 100;
    if (v >= 3.95f) return 80 + (v - 3.95f) * 80;
    if (v >= 3.85f) return 60 + (v - 3.85f) * 200;
    if (v >= 3.75f) return 40 + (v - 3.75f) * 200;
    if (v >= 3.65f) return 20 + (v - 3.65f) * 200;
    if (v >= 3.50f) return 10 + (v - 3.50f) * 100;
    if (v >= 3.30f) return (v - 3.30f) * 33;

    return 0;
}
