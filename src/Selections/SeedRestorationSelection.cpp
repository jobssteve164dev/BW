#include "SeedRestorationSelection.h"

namespace selections {

SeedRestorationSelection::SeedRestorationSelection(CardputerView& display, CardputerInput& input)
    : display(display), input(input) {}

SeedRestorationModeEnum SeedRestorationSelection::select() {
    selectionIndex = 0;
    lastIndex = -1;
    char key = KEY_NONE;
    while (key != KEY_OK && key != KEY_ARROW_RIGHT) {
        if (lastIndex != selectionIndex) {
            display.displaySelection(getModeStrings(), selectionIndex, getModeDescriptionStrings());
            lastIndex = selectionIndex;
        }

        key = input.handler();
        switch (key) {
            case KEY_ARROW_DOWN:
                selectionIndex = (selectionIndex < static_cast<uint8_t>(SeedRestorationModeEnum::COUNT) - 1) ? selectionIndex + 1 : 0;
                break;
            case KEY_ARROW_UP:
                selectionIndex = (selectionIndex > 0) ? selectionIndex - 1 : static_cast<uint8_t>(SeedRestorationModeEnum::COUNT) - 1;
                break;
            case KEY_RETURN_CUSTOM:
                return SeedRestorationModeEnum::NONE;
        }
    }

    return static_cast<SeedRestorationModeEnum>(selectionIndex);
}

const std::string SeedRestorationSelection::getModeToString(SeedRestorationModeEnum mode) const {
    switch (mode) {
        case SeedRestorationModeEnum::RFID:
            return "RFID 标签";
        case SeedRestorationModeEnum::SD:
            return "SD 卡";
        case SeedRestorationModeEnum::WORDS_12:
            return "12 个单词";
        case SeedRestorationModeEnum::WORDS_24:
            return "24 个单词";
        default:
            return "未知";
    }
}

const std::string SeedRestorationSelection::getModeDescription(SeedRestorationModeEnum mode) const {
    switch (mode) {
        case SeedRestorationModeEnum::RFID:
            return "从 RFID 读取";
        case SeedRestorationModeEnum::SD:
            return "从 SD 卡读取";
        case SeedRestorationModeEnum::WORDS_12:
        case SeedRestorationModeEnum::WORDS_24:
            return "手动输入助记词";
        default:
            return "未知";
    }
}

const std::vector<std::string> SeedRestorationSelection::getModeStrings() const {
    std::vector<std::string> modeStrings;
    for (int i = 0; i < static_cast<int>(SeedRestorationModeEnum::COUNT); ++i) {
        modeStrings.push_back(getModeToString(static_cast<SeedRestorationModeEnum>(i)));
    }
    return modeStrings;
}

const std::vector<std::string> SeedRestorationSelection::getModeDescriptionStrings() const {
    std::vector<std::string> descriptionStrings;
    for (int i = 0; i < static_cast<int>(SeedRestorationModeEnum::COUNT); ++i) {
        descriptionStrings.push_back(getModeDescription(static_cast<SeedRestorationModeEnum>(i)));
    }
    return descriptionStrings;
}

} // namespace selections
