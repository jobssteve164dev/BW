#include <cassert>
#include <iostream>
#include <string>

#include "Controllers/WalletController.h"

using contexts::SelectionContext;
using controllers::WalletController;
using enums::WalletInformationEnum;
using managers::WalletManager;
using models::Wallet;

namespace {

void prepareWallet() {
    auto& context = SelectionContext::getInstance();
    context.reset();
    context.setCurrentSelectedWallet(Wallet(
        "主钱包",
        "bc1qcompleteaddress",
        "zpub-complete-public-key",
        "A1B2C3D4",
        "m/84'/0'/0'"
    ));
}

void assertViewingDoesNotTouchKeyboardOrUsb(WalletInformationEnum info,
                                             const std::string& expectedDescription,
                                             const std::string& expectedValue) {
    prepareWallet();
    WalletManager manager;
    manager.walletInformationSelection.next = info;
    WalletController controller(manager);

    controller.handleWalletInformationSelection();

    assert(manager.valueSelection.selectCalls == 1);
    assert(manager.valueSelection.lastDescription == expectedDescription);
    assert(manager.valueSelection.lastValue == expectedValue);
    assert(manager.keyboardLayoutSelection.selectCalls == 0);
    assert(manager.usbService.setLayoutCalls == 0);
    assert(manager.usbService.beginCalls == 0);
}

} // namespace

int main() {
    assertViewingDoesNotTouchKeyboardOrUsb(
        WalletInformationEnum::BALANCE,
        "余额",
        "https://balance.example/?q=zpub-complete-public-key"
    );
    assertViewingDoesNotTouchKeyboardOrUsb(
        WalletInformationEnum::ADDRESS,
        "地址",
        "bc1qcompleteaddress"
    );
    std::cout << "wallet controller value entry tests passed\n";
    return 0;
}
