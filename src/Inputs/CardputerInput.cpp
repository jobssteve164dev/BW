#include "CardputerInput.h"

namespace inputs {


bool CardputerInput::updateDisplayStandby() {
    // Update keyboard state
    M5Cardputer.update();

    const bool inputActive = M5Cardputer.BtnA.isPressed() ||
                             M5Cardputer.Keyboard.isPressed();
    const auto displayAction = idleDisplayPolicy.poll(millis(), inputActive);
    if (displayAction == services::IdleDisplayAction::TURN_OFF) {
        const auto currentBrightness = M5Cardputer.Display.getBrightness();
        awakeBrightness = currentBrightness == 0 ? 120 : currentBrightness;
        M5Cardputer.Display.setBrightness(0);
        return true;
    }
    if (displayAction == services::IdleDisplayAction::TURN_ON) {
        M5Cardputer.Display.setBrightness(awakeBrightness);
        return true;
    }
    if (displayAction == services::IdleDisplayAction::CONSUME_INPUT) {
        return true;
    }
    return false;
}

char CardputerInput::handler() {
    if (updateDisplayStandby()) {
        return KEY_NONE;
    }

    // Bouton GO
    if (M5Cardputer.BtnA.isPressed()) {
        delay(150); // debounce
        return KEY_RETURN_CUSTOM;
    }
    
    if (M5Cardputer.Keyboard.isChange()) {

        if (M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            entropyContext.tick();

            if (status.enter) { // go to next menu
                return KEY_OK;
            }
            if (status.del) { 
                return KEY_DEL;
            }
            
            if(M5Cardputer.Keyboard.isKeyPressed(KEY_ARROW_LEFT)) { // go back to previous menu
                return KEY_RETURN_CUSTOM;
            }

            if(M5Cardputer.Keyboard.isKeyPressed(KEY_ARROW_RIGHT)) { // go to next menu
                return KEY_ARROW_RIGHT;
            }

            for (auto c : status.word) {
                if (isalnum(c)) {entropyContext.add(c);} // get some entropy
                return c; // retourner le premier char saisi
            }
        }
    }
    delay(10); // debounce
    return KEY_NONE;
}

void CardputerInput::pollStandby() {
    updateDisplayStandby();
}

void CardputerInput::waitPress() {
  while(1){
    if (updateDisplayStandby()) {
      delay(5);
      continue;
    }
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        entropyContext.tick();
        for (auto c : status.word) {
            if (isalnum(c)) {entropyContext.add(c);} // get some entropy
        }
        return;
    }
    delay(5);
  }
}

}
