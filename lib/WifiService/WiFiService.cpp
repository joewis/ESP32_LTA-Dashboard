#include "WiFiService.h"

void WiFiService::begin(const Config& config)
{
    _config = config;

    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(_config.txPower);

    if (_config.useStaticIP) {
        WiFi.config(_config.localIP,
                    _config.gateway,
                    _config.subnet,
                    _config.dns1,
                    _config.dns2);
    }

    startConnection();
}

void WiFiService::startConnection()
{
    WiFi.begin(_config.ssid, _config.password);
    _state = State::Connecting;
    _connectStartMillis = millis();
}

void WiFiService::loop()
{
    switch (_state) {
        case State::Connecting:
            handleConnecting();
            break;

        case State::Connected:
            if (WiFi.status() != WL_CONNECTED) {
                _state = State::WaitingToRetry;
                _lastAttemptMillis = millis();
            }
            break;

        case State::WaitingToRetry:
            handleReconnectTimer();
            break;

        default:
            break;
    }
}

void WiFiService::handleConnecting()
{
    if (WiFi.status() == WL_CONNECTED) {
        _state = State::Connected;
        return;
    }

    // Timeout after 30 seconds
    if (millis() - _connectStartMillis > 30000) {
        WiFi.disconnect();
        _state = State::WaitingToRetry;
        _lastAttemptMillis = millis();
    }
}

void WiFiService::handleReconnectTimer()
{
    if (!_autoReconnect) return;

    if (millis() - _lastAttemptMillis >= _config.reconnectIntervalMs) {
        startConnection();
    }
}

void WiFiService::disconnect(bool powerOff)
{
    WiFi.disconnect(true);

    if (powerOff) {
        WiFi.mode(WIFI_OFF);
    }

    _state = State::Idle;
}

void WiFiService::requestReconnect()
{
    disconnect(false);
    startConnection();
}

bool WiFiService::isConnected() const
{
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiService::isConnecting() const
{
    return _state == State::Connecting;
}

void WiFiService::enableAutoReconnect(bool enable)
{
    _autoReconnect = enable;
}

void WiFiService::setPowerSave(bool enabled)
{
    if (enabled) {
        WiFi.setSleep(true);
    } else {
        WiFi.setSleep(false);
    }
}

void WiFiService::setTxPower(wifi_power_t power)
{
    WiFi.setTxPower(power);
}

IPAddress WiFiService::localIP() const
{
    return WiFi.localIP();
}

int WiFiService::getRSSI() const
{
    if (WiFi.status() != WL_CONNECTED)
        return 0;

    return WiFi.RSSI();
}