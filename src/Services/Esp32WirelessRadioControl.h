#ifndef ESP32_WIRELESS_RADIO_CONTROL_H
#define ESP32_WIRELESS_RADIO_CONTROL_H

#include "WirelessSecurity.h"

namespace services {

class Esp32WifiRadio : public WifiRadio {
public:
    bool disable() override;
    RadioState state() const override;
};

class Esp32BluetoothRadio : public BluetoothRadio {
public:
    bool disable() override;
    RadioState state() const override;

private:
    bool memoryReleased = false;
};

} // namespace services

#endif // ESP32_WIRELESS_RADIO_CONTROL_H
