#ifndef CARDPUTER_KEYBOARD_INPUT_H
#define CARDPUTER_KEYBOARD_INPUT_H

#include <map> 
#include <M5Cardputer.h>
#include <Contexts/EntropyContext.h>
#include <Services/IdleDisplayPolicy.h>

#define KEY_OK '\n'
#define KEY_DEL '\b'
#define KEY_ESC_CUSTOM '`'
#define KEY_NONE '\0'
#define KEY_RETURN_CUSTOM '\r'
#define KEY_ARROW_UP ';'
#define KEY_ARROW_DOWN '.'
#define KEY_ARROW_LEFT ','
#define KEY_ARROW_RIGHT '/'

using namespace contexts;

namespace inputs {

constexpr uint32_t DISPLAY_STANDBY_TIMEOUT_MS = 120000;

class CardputerInput {
public:
    char handler();
    void waitPress();
    void pollStandby();
private:
    bool updateDisplayStandby();
    EntropyContext& entropyContext = EntropyContext::getInstance();
    services::IdleDisplayPolicy idleDisplayPolicy {DISPLAY_STANDBY_TIMEOUT_MS};
    uint8_t awakeBrightness = 120;
};

}


#endif // KEYBOARD_H
