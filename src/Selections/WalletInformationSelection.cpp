#include "WalletInformationSelection.h"

namespace selections {

WalletInformationSelection::WalletInformationSelection(CardputerView& display, CardputerInput& input)
    : display(display), input(input), selectionIndex(0), lastIndex(-1) {}

WalletInformationEnum WalletInformationSelection::select(const std::string& walletName) {
    display.displayTopBar(walletName.c_str(), true, false, true, 10);
    char key = KEY_NONE;
    lastIndex = -1;

    while (key != KEY_OK && key != KEY_ARROW_RIGHT) {
        if (lastIndex != selectionIndex) {
            display.displaySelection(getWalletInformationStrings(), selectionIndex);
            lastIndex = selectionIndex;

            if (selectionIndex > 3) { // on the second page
                display.displayPlugUsbMention();
            }
        }

        key = input.handler();

        switch (key) {
            case KEY_ARROW_DOWN:
                selectionIndex = (selectionIndex < static_cast<uint8_t>(WalletInformationEnum::COUNT) - 1) ? selectionIndex + 1 : 0;
                break;
            case KEY_ARROW_UP:
                selectionIndex = (selectionIndex > 0) ? selectionIndex - 1 : static_cast<uint8_t>(WalletInformationEnum::COUNT) - 1;
                break;
            case KEY_RETURN_CUSTOM:
                selectionIndex = 0; // default
                return WalletInformationEnum::NONE;
        }
    }

    return static_cast<WalletInformationEnum>(selectionIndex);
}

const std::string WalletInformationSelection::getWalletInformationToString(WalletInformationEnum info) {
    switch (info) {
        case WalletInformationEnum::BALANCE:
            return "比特币余额";
        case WalletInformationEnum::ADDRESS:
            return "比特币地址";
        case WalletInformationEnum::SIGNATURE:
            return "签名交易";
        case WalletInformationEnum::PUBLIC_KEY:
            return "钱包公钥 ZPUB";
        case WalletInformationEnum::FINGERPRINT:
            return "主密钥指纹";
        case WalletInformationEnum::DERIVE_PATH:
            return "派生路径";
        default:
            return "未知";
    }
}

const std::vector<std::string> WalletInformationSelection::getWalletInformationStrings() {
    std::vector<std::string> infoStrings;
    for (int i = 0; i < static_cast<int>(WalletInformationEnum::COUNT); ++i) {
        infoStrings.push_back(getWalletInformationToString(static_cast<WalletInformationEnum>(i)));
    }
    return infoStrings;
}

}
