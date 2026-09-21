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
    services::RadioState currentState = services::RadioState::ACTIVE;
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
    services::RadioState currentState = services::RadioState::ACTIVE;
};

void testAllowsStartupOnlyWhenBothRadiosAreDisabled() {
    FakeWifiRadio wifi(true, services::RadioState::ISOLATED);
    FakeBluetoothRadio bluetooth(true, services::RadioState::ISOLATED);

    assert(services::enforceWirelessIsolation(wifi, bluetooth));
    assert(wifi.attempted);
    assert(bluetooth.attempted);
}

void testBlocksStartupWhenACommandFailsEvenIfFinalStateLooksDisabled() {
    FakeWifiRadio wifiFailure(false, services::RadioState::ISOLATED);
    FakeBluetoothRadio bluetoothAfterWifiFailure(true, services::RadioState::ISOLATED);
    assert(!services::enforceWirelessIsolation(wifiFailure, bluetoothAfterWifiFailure));

    FakeWifiRadio wifiAfterBluetoothFailure(true, services::RadioState::ISOLATED);
    FakeBluetoothRadio bluetoothFailure(false, services::RadioState::ISOLATED);
    assert(!services::enforceWirelessIsolation(wifiAfterBluetoothFailure, bluetoothFailure));
}

void testBlocksStartupWhenAStateIsEnabledOrUnknown() {
    FakeWifiRadio wifiStillEnabled(true, services::RadioState::ACTIVE);
    FakeBluetoothRadio bluetoothDisabled(true, services::RadioState::ISOLATED);
    assert(!services::enforceWirelessIsolation(wifiStillEnabled, bluetoothDisabled));

    FakeWifiRadio wifiUnknown(true, services::RadioState::INDETERMINATE);
    FakeBluetoothRadio anotherBluetoothDisabled(true, services::RadioState::ISOLATED);
    assert(!services::enforceWirelessIsolation(wifiUnknown, anotherBluetoothDisabled));
}

void testAttemptsBluetoothShutdownAfterWifiFailure() {
    FakeWifiRadio wifi(false, services::RadioState::INDETERMINATE);
    FakeBluetoothRadio bluetooth(true, services::RadioState::ISOLATED);

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
