#include <cassert>
#include <cstdint>
#include <iostream>

#include "Services/BatteryStatus.h"
#include "Services/IdleDisplayPolicy.h"

using services::BatteryStatus;
using services::IdleDisplayAction;
using services::IdleDisplayPolicy;

namespace {

void testDisplayTurnsOffOnlyAfterContinuousInactivity() {
    IdleDisplayPolicy policy(120000);

    assert(policy.poll(5000, false) == IdleDisplayAction::NONE);
    assert(policy.poll(124999, false) == IdleDisplayAction::NONE);
    assert(policy.poll(125000, false) == IdleDisplayAction::TURN_OFF);
    assert(policy.isSleeping());
    assert(policy.poll(126000, false) == IdleDisplayAction::NONE);
}

void testInputResetsTheStandbyDeadline() {
    IdleDisplayPolicy policy(120000);

    assert(policy.poll(1000, false) == IdleDisplayAction::NONE);
    assert(policy.poll(120000, true) == IdleDisplayAction::NONE);
    assert(policy.poll(239999, false) == IdleDisplayAction::NONE);
    assert(policy.poll(240000, false) == IdleDisplayAction::TURN_OFF);
}

void testWakeInputIsConsumedUntilEveryKeyIsReleased() {
    IdleDisplayPolicy policy(1000);

    assert(policy.poll(10, false) == IdleDisplayAction::NONE);
    assert(policy.poll(1010, false) == IdleDisplayAction::TURN_OFF);
    assert(policy.poll(1100, true) == IdleDisplayAction::TURN_ON);
    assert(policy.poll(1110, true) == IdleDisplayAction::CONSUME_INPUT);
    assert(policy.poll(1120, false) == IdleDisplayAction::NONE);
    assert(policy.poll(1130, true) == IdleDisplayAction::NONE);
}

void testMillisWrapDoesNotTriggerEarlyStandby() {
    IdleDisplayPolicy policy(100);

    assert(policy.poll(UINT32_MAX - 49, false) == IdleDisplayAction::NONE);
    assert(policy.poll(25, false) == IdleDisplayAction::NONE);
    assert(policy.poll(50, false) == IdleDisplayAction::TURN_OFF);
}

void testBatteryStatusClampsReadingsAndHandlesUnavailableSensor() {
    assert(BatteryStatus::percent(-1) == -1);
    assert(BatteryStatus::label(-1) == "--%");
    assert(BatteryStatus::percent(0) == 0);
    assert(BatteryStatus::label(7) == "7%");
    assert(BatteryStatus::percent(105) == 100);
    assert(BatteryStatus::label(105) == "100%");
    assert(BatteryStatus::fillWidth(50, 16) == 8);
}

} // namespace

int main() {
    testDisplayTurnsOffOnlyAfterContinuousInactivity();
    testInputResetsTheStandbyDeadline();
    testWakeInputIsConsumedUntilEveryKeyIsReleased();
    testMillisWrapDoesNotTriggerEarlyStandby();
    testBatteryStatusClampsReadingsAndHandlesUnavailableSensor();
    std::cout << "power management tests passed\n";
    return 0;
}
