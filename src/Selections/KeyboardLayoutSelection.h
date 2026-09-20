#ifndef KEYBOARDLAYOUTSELECTION_H
#define KEYBOARDLAYOUTSELECTION_H

#include <vector>
#include <string>
#include <Arduino.h>
#include <Views/CardputerView.h>
#include <Inputs/CardputerInput.h>
#include <USBHIDKeyboard.h>

using namespace views;
using namespace inputs;

namespace selections {

class KeyboardLayoutSelection {
public:
    KeyboardLayoutSelection(CardputerView& display, CardputerInput& input);

    // Affiche les layouts et renvoie le pointeur du layout selectionné
    const uint8_t* select();

private:
    CardputerView& display;
    CardputerInput& input;
    size_t currentIndex = 0;

    std::vector<std::pair<std::string, const uint8_t*>> layouts = {
        {"英语（美国）", KeyboardLayout_en_US},
        {"法语（法国）", KeyboardLayout_fr_FR},
        {"德语（德国）", KeyboardLayout_de_DE},
        {"西班牙语", KeyboardLayout_es_ES},
        {"意大利语", KeyboardLayout_it_IT},
        {"葡萄牙语", KeyboardLayout_pt_PT},
        {"葡萄牙语（巴西）", KeyboardLayout_pt_BR},
        {"瑞典语", KeyboardLayout_sv_SE},
        {"丹麦语", KeyboardLayout_da_DK},
        {"匈牙利语", KeyboardLayout_hu_HU}
    };
};

} // namespace selections

#endif // KEYBOARDLAYOUTSELECTION_H
