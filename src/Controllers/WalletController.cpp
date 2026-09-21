#include "WalletController.h"

namespace controllers {

WalletController::WalletController(WalletManager& manager) : manager(manager) {}

void WalletController::handleWalletSelection() {
    auto wallets = manager.walletService.getAllWallets();

    // No wallets currently in the repo, ask for loading wallets file from the SD card
    if (wallets.empty()) {
        auto confirmation = manager.confirmationSelection.select("加载钱包文件？");

        if (confirmation) {
            selectionContext.requestIndexedWalletLoad();
        } else {
            selectionContext.setIsModeSelected(false); // Go back to menu
        }
        return;
    }

    // Select the desired wallet
    auto selectedWallet = manager.walletSelection.select(wallets);
    
    // User hits the return button if the returned wallet is empty
    if (selectedWallet.empty()) {
        selectionContext.setIsModeSelected(false); // Go back to menu
        return;
    }
    
    // Go to the next step
    selectionContext.setCurrentSelectedWallet(selectedWallet);
    selectionContext.setIsWalletSelected(true);
}

void WalletController::handleWalletInformationSelection() {
    // Get the selected wallet
    auto selectedWallet = selectionContext.getCurrentSelectedWallet();
    // Select between Balance, Btc Address, PubKey, Sign
    auto selectedInfo = manager.walletInformationSelection.select(selectedWallet.getName());
    
    // Route to the selected wallet infos
    switch (selectedInfo) {
        case WalletInformationEnum::NONE: // when key return is hits
            manager.endTransactionSigning();
            selectionContext.setIsWalletSelected(false); // go back to wallet selection
            break;

        case WalletInformationEnum::BALANCE:
            manager.valueSelection.select(
                "余额",
                globalContext.getBitcoinBalanceUrl() + selectedWallet.getZPub(), 
                manager.usbService,
                manager.ledService,
                manager.keyboardLayoutSelection
            );
            break;

        case WalletInformationEnum::ADDRESS:
            manager.valueSelection.select(
                "地址",
                selectedWallet.getAddress(), 
                manager.usbService,
                manager.ledService,
                manager.keyboardLayoutSelection
            );
            break;

        case WalletInformationEnum::SIGNATURE: {
            selectionContext.getTransactionSigningFlow().begin();
            selectionContext.setTransactionOngoing(true);
            selectionContext.setCurrentSelectedMode(SelectionModeEnum::LOAD_SD);
            selectionContext.setCurrentSelectedFileType(FileTypeEnum::TRANSACTION);
            manager.display.displaySubMessage("选择待签名 PSBT", 43, 1800);
            break;
        }

        case WalletInformationEnum::PUBLIC_KEY:
            manager.valueSelection.select(
                "公钥 Zpub",
                selectedWallet.getZPub(), 
                manager.usbService,
                manager.ledService,
                manager.keyboardLayoutSelection
            );
            break;

        case WalletInformationEnum::FINGERPRINT:
            manager.valueSelection.select(
                "主密钥指纹",
                selectedWallet.getFingerprint(), 
                manager.usbService,
                manager.ledService,
                manager.keyboardLayoutSelection
            );
            break;

        case WalletInformationEnum::DERIVE_PATH:
            manager.valueSelection.select(
                "派生路径",
                selectedWallet.getDerivePath(), 
                manager.usbService,
                manager.ledService,
                manager.keyboardLayoutSelection
            );
            break;
    }
}

} // namespace controllers
