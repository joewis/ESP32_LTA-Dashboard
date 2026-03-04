#pragma once
#include <Arduino.h>
#include <time.h>
#include "TimeService.h"

class TimeService {
public:
    void begin(const char* server1,
               const char* server2,
               const char* server3,
               const char* tzString,
               uint32_t refreshIntervalMs = 0);

    void loop();                        // non-blocking handler
    void requestRefresh();              // manual refresh trigger

    bool isSynchronized() const;

    // Raw time access
    bool getTime(struct tm& outTime) const;
    time_t getEpoch() const;

    /**
     * Format current time into user-provided buffer.
     *
     * @param buffer        Output buffer
     * @param bufferSize    Size of buffer
     * @param formatString  strftime-style format string
     *
     * Returns true on success.
     *
     * Supported format specifiers (POSIX strftime):
     *
     *   %Y  Year with century        (2026)
     *   %y  Year without century     (26)
     *
     *   %m  Month number (01-12)
     *   %B  Full month name          (March)
     *   %b  Abbreviated month name   (Mar)
     *
     *   %d  Day of month (01-31)
     *   %e  Day of month (1-31)
     *
     *   %A  Full weekday name        (Tuesday)
     *   %a  Abbreviated weekday      (Tue)
     *
     *   %H  Hour (24h, 00-23)
     *   %I  Hour (12h, 01-12)
     *   %M  Minute (00-59)
     *   %S  Second (00-59)
     *
     *   %p  AM/PM
     *
     *   %j  Day of year (001-366)
     *   %w  Weekday number (0=Sunday)
     *
     *   %F  ISO date shortcut        (%Y-%m-%d)
     *   %T  ISO time shortcut        (%H:%M:%S)
     *
     * Example:
     *
     *   char buf[32];
     *   timeService.format(buf, sizeof(buf), "%Y-%m-%d %H:%M");
     *
     */
    bool format(char* buffer,
                size_t bufferSize,
                const char* formatString) const;

private:
    enum class State {
        Idle,
        WaitingForSync
    };

    void startSync();
    void handleSync();

    const char* _server1;
    const char* _server2;
    const char* _server3;

    uint32_t _refreshInterval;
    uint32_t _lastSyncMillis = 0;
    uint32_t _syncStartMillis = 0;

    bool _synchronized = false;
    State _state = State::Idle;
};