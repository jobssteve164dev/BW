#ifndef WIRELESS_SECURITY_H
#define WIRELESS_SECURITY_H

namespace services {

enum class RadioState {
    DISABLED,
    ENABLED,
    UNKNOWN
};

class WifiRadio {
public:
    virtual ~WifiRadio() {}
    virtual bool disable() = 0;
    virtual RadioState state() const = 0;
};

class BluetoothRadio {
public:
    virtual ~BluetoothRadio() {}
    virtual bool disable() = 0;
    virtual RadioState state() const = 0;
};

bool enforceWirelessIsolation(WifiRadio& wifi, BluetoothRadio& bluetooth);

} // namespace services

#endif // WIRELESS_SECURITY_H
