#include "StringPromptSelection.h"

namespace selections {

StringPromptSelection::StringPromptSelection(CardputerView& display, CardputerInput& input)
    : display(display), input(input) {}

std::string StringPromptSelection::select(std::string description,
                                          size_t offsetX,
                                          bool backButton,
                                          bool password,
                                          size_t minimumLength) {
    std::string output;
    char key = KEY_NONE;
    auto limit = globalContext.getMaxInputCharCount();
    display.displayStringPrompt(description, output, offsetX, backButton, password, minimumLength);

    if (password) {
        limit = globalContext.getMaxInputCharPasswordCount();
    }

    while (key != KEY_OK || output.length() < minimumLength) {
        key = input.handler();
        if (key == KEY_DEL) {
            if (!output.empty()) {
                output.pop_back();
            }
        }
        else if (key == KEY_RETURN_CUSTOM && backButton) {
            if (password) {
                volatile char* data = output.empty() ? nullptr : &output[0];
                for (size_t index = 0; index < output.size(); ++index) {
                    data[index] = 0;
                }
                output.clear();
            }
            return ""; // empty string will not save
        }
        else if (isprint(key) && output.size() < limit) {
            output += key;
        }

        if (key != KEY_NONE) {
            display.displayStringPrompt(description, output, offsetX, backButton, password, minimumLength);
        }
    }
    return output;
}

}
