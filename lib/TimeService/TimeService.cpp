#include "TimeService.h"

void TimeService::begin(const char* server1,
                        const char* server2,
                        const char* server3,
                        const char* tzString,
                        uint32_t refreshIntervalMs)
{
    _server1 = server1;
    _server2 = server2;
    _server3 = server3;
    _refreshInterval = refreshIntervalMs;

    setenv("TZ", tzString, 1);
    tzset();

    configTime(0, 0, _server1, _server2, _server3);

    startSync();   // non-blocking
}

void TimeService::startSync()
{
    _state = State::WaitingForSync;
    _syncStartMillis = millis();
}

void TimeService::requestRefresh()
{
    startSync();
}

void TimeService::loop()
{
    if (_state == State::WaitingForSync) {
        handleSync();
    }

    // periodic refresh
    if (_refreshInterval > 0 &&
        _synchronized &&
        millis() - _lastSyncMillis > _refreshInterval)
    {
        startSync();
    }
}

void TimeService::handleSync()
{
    struct tm timeInfo;

    if (getLocalTime(&timeInfo, 0)) {   // 0 timeout = non-blocking
        _synchronized = true;
        _lastSyncMillis = millis();
        _state = State::Idle;
        return;
    }

    // Optional: timeout after 10 seconds
    if (millis() - _syncStartMillis > 10000) {
        _state = State::Idle;
    }
}

bool TimeService::isSynchronized() const
{
    return _synchronized;
}

bool TimeService::getTime(struct tm& outTime) const
{
    if (!_synchronized) return false;
    return getLocalTime(&outTime, 0);
}

time_t TimeService::getEpoch() const
{
    if (!_synchronized) return 0;
    return time(nullptr);
}

bool TimeService::format(char* buffer,
                         size_t bufferSize,
                         const char* formatString) const
{
    if (!_synchronized) return false;

    struct tm timeInfo;
    if (!getLocalTime(&timeInfo, 0))
        return false;

    size_t written = strftime(buffer,
                              bufferSize,
                              formatString,
                              &timeInfo);

    return written > 0;
}