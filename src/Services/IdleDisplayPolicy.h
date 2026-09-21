#ifndef IDLE_DISPLAY_POLICY_H
#define IDLE_DISPLAY_POLICY_H

#include <cstdint>

namespace services {

enum class IdleDisplayAction {
    NONE,
    TURN_OFF,
    TURN_ON,
    CONSUME_INPUT,
};

class IdleDisplayPolicy {
public:
    explicit IdleDisplayPolicy(uint32_t timeoutMilliseconds)
        : timeoutMilliseconds(timeoutMilliseconds) {}

    IdleDisplayAction poll(uint32_t now, bool inputActive) {
        if (!initialized) {
            initialized = true;
            lastActivity = now;
        }

        if (sleeping) {
            if (inputActive) {
                sleeping = false;
                suppressInputUntilRelease = true;
                lastActivity = now;
                return IdleDisplayAction::TURN_ON;
            }
            return IdleDisplayAction::NONE;
        }

        if (suppressInputUntilRelease) {
            if (inputActive) {
                return IdleDisplayAction::CONSUME_INPUT;
            }
            suppressInputUntilRelease = false;
            return IdleDisplayAction::NONE;
        }

        if (inputActive) {
            lastActivity = now;
            return IdleDisplayAction::NONE;
        }

        if (timeoutMilliseconds > 0 && now - lastActivity >= timeoutMilliseconds) {
            sleeping = true;
            return IdleDisplayAction::TURN_OFF;
        }
        return IdleDisplayAction::NONE;
    }

    bool isSleeping() const { return sleeping; }

private:
    uint32_t timeoutMilliseconds;
    uint32_t lastActivity = 0;
    bool initialized = false;
    bool sleeping = false;
    bool suppressInputUntilRelease = false;
};

} // namespace services

#endif // IDLE_DISPLAY_POLICY_H
