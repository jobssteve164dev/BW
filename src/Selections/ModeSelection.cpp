#include "ModeSelection.h"

namespace selections {

ModeSelection::ModeSelection(CardputerView& display, CardputerInput& input)
    : display(display), input(input) {}

SelectionModeEnum ModeSelection::select() {
    display.displayTopBar(globalContext.getAppName());
    display.drawBitcoinIcon(20, 3);
    display.displaySelection(getSelectionModeStrings(), selectionIndex, getSelectionModeDescriptionStrings());
    selectionIndex = 0;
    lastIndex = -1;
    
    char key = KEY_NONE;
    while (key != KEY_OK && key != KEY_ARROW_RIGHT) {
        if (lastIndex != selectionIndex) {
            display.displaySelection(getSelectionModeStrings(), selectionIndex, getSelectionModeDescriptionStrings());
            lastIndex = selectionIndex;
        }

        key = input.handler();
        switch (key) {
            case KEY_ARROW_DOWN:
                selectionIndex = (selectionIndex < static_cast<uint8_t>(SelectionModeEnum::COUNT) - 1) ? selectionIndex + 1 : 0;
                break;
            case KEY_ARROW_UP:
                selectionIndex = (selectionIndex > 0) ? selectionIndex - 1 : static_cast<uint8_t>(SelectionModeEnum::COUNT) - 1;
                break;
        }
    }

    return static_cast<SelectionModeEnum>(selectionIndex);
}


const std::string ModeSelection::getSelectionModeToString(SelectionModeEnum mode) {
    switch (mode) {
        case SelectionModeEnum::PORTFOLIO:
            return "钱包列表";
        case SelectionModeEnum::CREATE_WALLET:
            return "创建钱包";
        case SelectionModeEnum::LOAD_SD:
            return "加载钱包";
        case SelectionModeEnum::LOAD_SEED:
            return "恢复助记词";
        case SelectionModeEnum::INFOS:
            return "使用说明";
        default:
            return "未知";
    }
}

const std::string ModeSelection::getSelectionModeDescription(SelectionModeEnum mode) {
    switch (mode) {
        case SelectionModeEnum::PORTFOLIO:
            return "查看已保存钱包";
        case SelectionModeEnum::CREATE_WALLET:
            return "生成新助记词";
        case SelectionModeEnum::LOAD_SD:
            return "从 SD 卡读取";
        case SelectionModeEnum::LOAD_SEED:
            return "导入已有助记词";
        case SelectionModeEnum::INFOS:
            return "";
        default:
            return "未知";
    }
}

const std::vector<std::string> ModeSelection::getSelectionModeStrings() {
    std::vector<std::string> modeStrings;
    for (int i = 0; i < static_cast<int>(SelectionModeEnum::COUNT); ++i) {
        modeStrings.push_back(getSelectionModeToString(static_cast<SelectionModeEnum>(i)));
    }
    return modeStrings;
}

const std::vector<std::string> ModeSelection::getSelectionModeDescriptionStrings() {
    std::vector<std::string> descriptionStrings;
    for (int i = 0; i < static_cast<int>(SelectionModeEnum::COUNT); ++i) {
        descriptionStrings.push_back(getSelectionModeDescription(static_cast<SelectionModeEnum>(i)));
    }
    return descriptionStrings;
}

}
