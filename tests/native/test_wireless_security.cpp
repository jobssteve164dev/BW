#include <cassert>
#include <iostream>

#include "Services/WirelessSecurity.h"

namespace {

class FakeWifiRadio : public services::WifiRadio {
public:
    FakeWifiRadio(bool commandSucceeds, services::RadioState finalState)
        : commandSucceeds(commandSucceeds), finalState(finalState) {}

    bool disable() override {
        attempted = true;
        currentState = finalState;
        return commandSucceeds;
    }

    services::RadioState state() const override {
        return currentState;
    }

    bool attempted = false;

private:
    bool commandSucceeds;
    services::RadioState finalState;
    services::RadioState currentState = services::RadioState::ENABLED;
};

class FakeBluetoothRadio : public services::BluetoothRadio {
public:
    FakeBluetoothRadio(bool commandSucceeds, services::RadioState finalState)
        : commandSucceeds(commandSucceeds), finalState(finalState) {}

    bool disable() override {
        attempted = true;
        currentState = finalState;
        return commandSucceeds;
    }

    services::RadioState state() const override {
        return currentState;
    }

    bool attempted = false;

private:
    bool commandSucceeds;
    services::RadioState finalState;
    services::RadioState currentState = services::RadioState::ENABLED;
};

void testAllowsStartupOnlyWhenBothRadiosAreDisabled() {
    FakeWifiRadio wifi(true, services::RadioState::DISABLED);
    FakeBluetoothRadio bluetooth(true, services::RadioState::DISABLED);

    assert(services::enforceWirelessIsolation(wifi, bluetooth));
    assert(wifi.attempted);
    assert(bluetooth.attempted);
}

void testBlocksStartupWhenACommandFailsEvenIfFinalStateLooksDisabled() {
    FakeWifiRadio wifiFailure(false, services::RadioState::DISABLED);
    FakeBluetoothRadio bluetoothAfterWifiFailure(true, services::RadioState::DISABLED);
    assert(!services::enforceWirelessIsolation(wifiFailure, bluetoothAfterWifiFailure));

    FakeWifiRadio wifiAfterBluetoothFailure(true, services::RadioState::DISABLED);
    FakeBluetoothRadio bluetoothFailure(false, services::RadioState::DISABLED);
    assert(!services::enforceWirelessIsolation(wifiAfterBluetoothFailure, bluetoothFailure));
}

void testBlocksStartupWhenAStateIsEnabledOrUnknown() {
    FakeWifiRadio wifiStillEnabled(true, services::RadioState::ENABLED);
    FakeBluetoothRadio bluetoothDisabled(true, services::RadioState::DISABLED);
    assert(!services::enforceWirelessIsolation(wifiStillEnabled, bluetoothDisabled));

    FakeWifiRadio wifiUnknown(true, services::RadioState::UNKNOWN);
    FakeBluetoothRadio anotherBluetoothDisabled(true, services::RadioState::DISABLED);
    assert(!services::enforceWirelessIsolation(wifiUnknown, anotherBluetoothDisabled));
}

void testAttemptsBluetoothShutdownAfterWifiFailure() {
    FakeWifiRadio wifi(false, services::RadioState::UNKNOWN);
    FakeBluetoothRadio bluetooth(true, services::RadioState::DISABLED);

    assert(!services::enforceWirelessIsolation(wifi, bluetooth));
    assert(wifi.attempted);
    assert(bluetooth.attempted);
}

} // namespace

int main() {
    testAllowsStartupOnlyWhenBothRadiosAreDisabled();
    testBlocksStartupWhenACommandFailsEvenIfFinalStateLooksDisabled();
    testBlocksStartupWhenAStateIsEnabledOrUnknown();
    testAttemptsBluetoothShutdownAfterWifiFailure();
    std::cout << "wireless security tests passed\n";
    return 0;
}
