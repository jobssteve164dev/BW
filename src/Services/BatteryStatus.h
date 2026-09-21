#ifndef BATTERY_STATUS_H
#define BATTERY_STATUS_H

#include <algorithm>
#include <string>

namespace services {

class BatteryStatus {
public:
    static int percent(int reading) {
        if (reading < 0) {
            return -1;
        }
        return std::min(reading, 100);
    }

    static std::string label(int reading) {
        const int value = percent(reading);
        return value < 0 ? "--%" : std::to_string(value) + "%";
    }

    static int fillWidth(int reading, int maximumWidth) {
        const int value = percent(reading);
        if (value < 0 || maximumWidth <= 0) {
            return 0;
        }
        return maximumWidth * value / 100;
    }
};

} // namespace services

#endif // BATTERY_STATUS_H
