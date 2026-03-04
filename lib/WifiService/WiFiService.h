#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "WiFiService.h"

class WiFiService {
public:
    struct Config {
        const char* ssid;
        const char* password;

        bool useStaticIP = false;
        IPAddress localIP;
        IPAddress gateway;
        IPAddress subnet;
        IPAddress dns1;
        IPAddress dns2;

        wifi_power_t txPower = WIFI_POWER_19_5dBm;
        uint32_t reconnectIntervalMs = 10000;   // retry interval
    };

    void begin(const Config& config);
    void loop();                        // non-blocking handler

    void disconnect(bool powerOff = false);
    void requestReconnect();

    bool isConnected() const;
    bool isConnecting() const;

    void enableAutoReconnect(bool enable);

    void setPowerSave(bool enabled);
    void setTxPower(wifi_power_t power);

    IPAddress localIP() const;
    int getRSSI() const;

private:
    enum class State {
        Idle,
        Connecting,
        Connected,
        WaitingToRetry
    };

    void startConnection();
    void handleConnecting();
    void handleReconnectTimer();

    Config _config;
    State _state = State::Idle;

    bool _autoReconnect = true;
    uint32_t _lastAttemptMillis = 0;
    uint32_t _connectStartMillis = 0;
};