#include "WirelessSecurity.h"

namespace services {

bool enforceWirelessIsolation(WifiRadio& wifi, BluetoothRadio& bluetooth) {
    const bool wifiCommandSucceeded = wifi.disable();
    const bool bluetoothCommandSucceeded = bluetooth.disable();

    return wifiCommandSucceeded &&
           bluetoothCommandSucceeded &&
           wifi.state() == RadioState::DISABLED &&
           bluetooth.state() == RadioState::DISABLED;
}

} // namespace services
