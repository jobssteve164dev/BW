#include "FileBrowserManager.h"

namespace managers {
namespace {

void clearString(std::string& value) {
    volatile char* data = value.empty() ? nullptr : &value[0];
    for (size_t index = 0; index < value.size(); ++index) {
        data[index] = 0;
    }
    value.clear();
}

void clearBytes(std::vector<uint8_t>& value) {
    volatile uint8_t* data = value.empty() ? nullptr : value.data();
    for (size_t index = 0; index < value.size(); ++index) {
        data[index] = 0;
    }
    value.clear();
}

} // namespace

FileBrowserManager::FileBrowserManager(const GlobalManager& gm)
    : GlobalManager(gm)  // Call the base class (GlobalManager) copy constructor
{} 

bool FileBrowserManager::loadFile(std::string currentPath, FileTypeEnum selectedFileType) {
    switch (selectedFileType) {
        case FileTypeEnum::WALLET:
            return manageWalletFile(currentPath);
        case FileTypeEnum::SEED:
            if (selectionContext.getTransactionOngoing()) {
                return manageSeedLoadingFile(currentPath);
            } else {
                return manageSeedRestorationFile(currentPath);
            }
        case FileTypeEnum::TRANSACTION:
            return manageTransactionFile(currentPath);
    }
    return false;
}

bool FileBrowserManager::verifyWalletFile(const std::string& fileContent) {
    if (fileContent.find("Filetype: Card Wallet") != std::string::npos) {
        if (fileContent.find("Version: 1") != std::string::npos) {
            display.displayFileVersionInfos();
            input.waitPress();
            display.displayTopBar("旧版文件", false, false, false, 15);
            return false;
        } else if (fileContent.find("Version: 2") != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool FileBrowserManager::verifySeedFile(const std::string& fileContent) {
    // Split fileContent into tokens by whitespace
    std::istringstream iss(fileContent);
    std::vector<std::string> words {
        std::istream_iterator<std::string>(iss),
        std::istream_iterator<std::string>()
    };

    // Verify the total number of words is either 12 or 24
    return (words.size() == 12 || words.size() == 24);
}

bool FileBrowserManager::manageWalletFile(const std::string& currentPath) {
    auto fileName = extractFilename(currentPath);
    auto fileExt = extractFileExtension(fileName);

    if (fileExt == "txt") {
        auto fileContent = sdService.readFile(currentPath.c_str());
        if (verifyWalletFile(fileContent) && walletService.loadAllWallets(fileContent)) {
            selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
            selectionContext.setIsWalletSelected(false);
            globalContext.setFileWalletPath(currentPath);
            const bool pathSaved = settingsService.saveWalletPath(currentPath);
            display.displaySubMessage(
                pathSaved ? "钱包已加载" : "钱包已加载，启动路径未保存",
                pathSaved ? 50 : 13,
                pathSaved ? 1000 : 2200);
            return true;
        } else {
            confirmationSelection.select("钱包文件无效");
        }
    } else {
        confirmationSelection.select("不支持此文件");
    }
    return false;
}

bool FileBrowserManager::manageTransactionFile(const std::string& currentPath) {
    auto fileName = extractFilename(currentPath);
    auto fileExt = extractFileExtension(fileName);

    if (fileExt == "psbt") {
        auto& signingFlow = selectionContext.getTransactionSigningFlow();
        auto signingWallet = selectionContext.getCurrentSelectedWallet();
        if (signingFlow.stage() == TransactionSigningStage::SELECT_PSBT &&
            !signingFlow.selectPsbt(currentPath, !signingWallet.getMnemonic().empty())) {
            display.displaySubMessage("无法选择此交易", 42, 1800);
            return false;
        }

        if (signingFlow.stage() == TransactionSigningStage::UNLOCK_SECRETS) {
            const auto unlockResult = manageVaultUnlock(signingWallet);
            if (unlockResult == VaultUnlockResult::UNLOCKED) {
                signingFlow.secretsLoaded();
            } else {
                // Keep the selected PSBT while the user chooses RFID, an SD
                // mnemonic file, or manual word entry.
                selectionContext.setCurrentSelectedMode(SelectionModeEnum::LOAD_SEED);
                return true;
            }
        }

        if (signingFlow.stage() != TransactionSigningStage::REVIEW_AND_SIGN ||
            signingFlow.selectedPsbtPath() != currentPath) {
            display.displaySubMessage("签名流程状态无效", 34, 1800);
            resetTransactionAttempt();
            return false;
        }

        display.displayTopBar("检查交易", false, false, true);
        display.displaySubMessage("正在加载", 83);

        // Read file
        auto fileContent = sdService.readBinaryFile(
            currentPath.c_str(), SdService::MAX_BINARY_FILE_SIZE);

        signingWallet = selectionContext.getCurrentSelectedWallet();
        TransactionReview review;
        if (!cryptoService.inspectBitcoinTransaction(
                fileContent,
                signingWallet.getMnemonic(),
                signingWallet.getPassphrase(),
                review)) {
            display.displaySubMessage("无法安全读取交易详情", 18, 2500);
            resetTransactionAttempt();
            return false;
        }
        if (!confirmTransaction(review)) {
            display.displaySubMessage("已取消签名", 60, 1500);
            resetTransactionAttempt();
            return false;
        }

        display.displayTopBar("正在签名", false, false, true);
        display.displaySubMessage("正在处理", 83);

        // Convert and get signature
        auto psbt = cryptoService.convertPSBTBinaryToBase64(fileContent);
        auto signedTransactionBytes = manageBitcoinSignature(psbt, signingWallet.getMnemonic());
        
        // Bad sign
        if (signedTransactionBytes.empty()) {
            signingWallet.clearSecrets();
            display.displaySubMessage("签名失败", 60, 2000);
            resetTransactionAttempt();
            return false;
        }

        std::vector<uint8_t> verifiedSignedTransaction;
        const bool signedTransactionMatches = cryptoService.mergeSignedBitcoinTransaction(
            fileContent, signedTransactionBytes, verifiedSignedTransaction);
        clearBytes(signedTransactionBytes);
        signingWallet.clearSecrets();
        if (!signedTransactionMatches) {
            display.displaySubMessage("签名结果校验失败", 38, 2500);
            resetTransactionAttempt();
            return false;
        }
        signedTransactionBytes.swap(verifiedSignedTransaction);
        clearLoadedWalletSecrets(selectionContext.getCurrentSelectedWallet());

        // Sign success, means it's the correct seed for the correct transaction
        display.displaySubMessage("签名成功", 25, 2000);
        
        // SD Save
        auto parent = getParentDirectory(currentPath);
        std::string baseFileName = fileName.substr(0, fileName.find_last_of('.')); // remove ext .psbt
        const auto signedPath = parent + "/" + baseFileName + "-signed.psbt";
        const auto temporarySignedPath = signedPath + ".tmp";
        const auto backupSignedPath = signedPath + ".bak";
        if (!sdService.replaceBinaryFile(
                signedPath.c_str(),
                temporarySignedPath.c_str(),
                backupSignedPath.c_str(),
                signedTransactionBytes)) {
            clearBytes(signedTransactionBytes);
            display.displaySubMessage("签名文件校验失败，原文件已保留", 5, 3000);
            signingFlow.begin();
            return false;
        }
        const bool unsignedRemoved = sdService.deleteFile(currentPath.c_str());
        removeCachedDirectoryElement(parent); // new sign.psbt in it, remove to refetch
        display.displaySubMessage(
            unsignedRemoved ? "签名已保存到 SD 卡" : "签名已保存，原文件未删除",
            unsignedRemoved ? 40 : 16,
            3000);

        if (confirmationSelection.select("显示签名二维码？")) {
            displaySignedTransactionQr(signedTransactionBytes);
        }
        clearBytes(signedTransactionBytes);
        
        // Check if user want to sign another tx
        auto signConfirmation = confirmationSelection.select("继续签名交易？");
        if (!signConfirmation) {
            // Go back to portfolio
            endTransactionSigning();
            selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
            return true;
        }

        signingFlow.begin();
        return false;

    } 

    confirmationSelection.select("不支持此文件");
    return false;
}

void FileBrowserManager::resetTransactionAttempt() {
    clearLoadedWalletSecrets(selectionContext.getCurrentSelectedWallet());
    selectionContext.getTransactionSigningFlow().begin();
}

bool FileBrowserManager::confirmTransaction(const TransactionReview& review) {
    for (size_t index = 0; index < review.outputs.size(); ++index) {
        const auto& output = review.outputs[index];
        display.displayTransactionOutput(
            index,
            review.outputs.size(),
            output.address,
            TransactionReviewService::formatBitcoin(output.satoshis));
        char key = KEY_NONE;
        while (key != KEY_OK && key != KEY_ARROW_RIGHT) {
            key = input.handler();
            if (key == KEY_RETURN_CUSTOM) {
                return false;
            }
        }
    }

    display.displayTransactionFee(
        TransactionReviewService::formatBitcoin(review.feeSatoshis),
        review.feeSatoshis);
    char key = KEY_NONE;
    while (key != KEY_OK && key != KEY_ARROW_RIGHT) {
        key = input.handler();
        if (key == KEY_RETURN_CUSTOM) {
            return false;
        }
    }
    return confirmationSelection.select("确认签名这笔交易？");
}

void FileBrowserManager::displaySignedTransactionQr(
    const std::vector<uint8_t>& signedTransaction) {
    std::vector<std::string> frames;
    if (!BbqrEncoder::encodePsbt(signedTransaction, frames)) {
        display.displaySubMessage("二维码生成失败", 46, 2000);
        return;
    }

    size_t frameIndex = 0;
    unsigned long lastFrameAt = 0;
    while (true) {
        const auto now = millis();
        if (lastFrameAt == 0 || now - lastFrameAt >= 500) {
            display.displayAnimatedQrFrame(frames[frameIndex], frameIndex, frames.size());
            frameIndex = (frameIndex + 1) % frames.size();
            lastFrameAt = now;
        }
        const auto key = input.handler();
        if (key == KEY_RETURN_CUSTOM || key == KEY_OK) {
            return;
        }
    }
}

bool FileBrowserManager::manageSeedLoadingFile(const std::string& currentPath) {
    auto fileName = extractFilename(currentPath);
    auto fileExt = extractFileExtension(fileName);
    std::string passphrase;

    if (fileExt == "txt") {
        if (!confirmationSelection.select("明文助记词不安全，继续？")) {
            return false;
        }
        auto fileContent = sdService.readFile(currentPath.c_str(), 512);
        if (verifySeedFile(fileContent)) {
            auto mnemonicString = fileContent;
            auto mnemonicWordList = cryptoService.mnemonicStringToWordList(fileContent);
            auto validation = cryptoService.verifyMnemonic(mnemonicWordList);

            if (!validation) {
                display.displaySubMessage("助记词无效", 41, 2000);
                clearString(mnemonicString);
                clearString(fileContent);
                return false;
            }

            display.displayTopBar("加载助记词", false, false, true, 5);
            display.displaySubMessage("助记词有效", 45, 2000);

            // Get Wallet
            auto wallet = selectionContext.getCurrentSelectedWallet();

            // Get seed passphrase
            auto passphrase = managePassphrase();

            // Derive PublicKey to check if seed match
            display.displaySubMessage("正在加载", 83);
            auto zPub = cryptoService.deriveZPub(mnemonicString, passphrase);
            if (zPub.toString().c_str() != wallet.getZPub()) {
                display.displaySubMessage("助记词与钱包不匹配", 18, 3000);
                clearString(mnemonicString);
                clearString(fileContent);
                clearString(passphrase);
                return false;
            }

            // Update
            display.displaySubMessage("助记词已加载", 65, 1500);
            wallet.setPassphrase(passphrase);
            wallet.setMnemonic(mnemonicString);
            selectionContext.setCurrentSelectedWallet(wallet);
            walletService.updateWallet(wallet);

            if (!selectionContext.getTransactionSigningFlow().secretsLoaded()) {
                endTransactionSigning();
                selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
                clearString(mnemonicString);
                clearString(fileContent);
                clearString(passphrase);
                return false;
            }

            // Resume the PSBT selected before secret loading.
            selectionContext.setCurrentSelectedMode(SelectionModeEnum::LOAD_SD);
            selectionContext.setCurrentSelectedFileType(FileTypeEnum::TRANSACTION);
            display.displaySubMessage("正在返回已选交易", 35, 1200);

            clearString(mnemonicString);
            clearString(fileContent);
            clearString(passphrase);
            return true;
        } else {
            confirmationSelection.select("助记词无效");
            clearString(fileContent);
        }
    } else {
        confirmationSelection.select("不支持此文件");
    }
    return false;
}

bool FileBrowserManager::manageSeedRestorationFile(const std::string& currentPath) {
    auto fileName = extractFilename(currentPath);
    auto fileExt = extractFileExtension(fileName);
    std::string passphrase;

    if (fileExt == "txt") {
        if (!confirmationSelection.select("明文助记词不安全，继续？")) {
            return false;
        }
        auto fileContent = sdService.readFile(currentPath.c_str(), 512);
        if (verifySeedFile(fileContent)) {
            auto mnemonicString = fileContent;
            auto mnemonicWordList = cryptoService.mnemonicStringToWordList(fileContent);
            auto validation = cryptoService.verifyMnemonic(mnemonicWordList);

            if (!validation) {
                display.displaySubMessage("助记词无效", 41, 2000);
                clearString(mnemonicString);
                clearString(fileContent);
                return false;
            }

            display.displayTopBar("恢复助记词", false, false, true, 5);
            display.displaySubMessage("助记词有效", 45, 2000);
            auto passphrase = managePassphrase(); // return "" in case user doesn't want passphrase
            auto privateKey = cryptoService.mnemonicToPrivateKey(mnemonicString);

            // Prompt for a wallet name
            display.displayTopBar("钱包", false, false, true);
            auto walletName = stringPromptSelection.select("输入钱包名称");
            if (walletName.empty()) {
                clearBytes(privateKey);
                clearString(mnemonicString);
                clearString(fileContent);
                clearString(passphrase);
                return false;
            }
            auto wallet = manageBitcoinWalletCreation(mnemonicString, passphrase, walletName);

            // Save wallet to SD if any
            display.displaySubMessage("正在加载", 83);
            sdService.begin(); // SD card start
            auto publicWalletSaved = manageSdSave(wallet);
            auto vaultSaved = manageVaultSave(privateKey, passphrase, wallet);

            // Optional additional RFID backup
            manageRfidSave(privateKey);
            display.displaySeedEnd(publicWalletSaved, vaultSaved);
            input.waitPress();
            sdService.close(); // SD card stop

            clearBytes(privateKey);
            clearString(mnemonicString);
            clearString(fileContent);
            clearString(passphrase);

            // Go to portfolio
            selectionContext.setIsWalletSelected(false);
            selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
            return true;
        } else {
            confirmationSelection.select("助记词无效");
            clearString(fileContent);
        }
    } else {
        confirmationSelection.select("不支持此文件");
    }
    return false;
}

std::string FileBrowserManager::extractFilename(const std::string& filepath) {
    size_t lastSlash = filepath.find_last_of("/\\");
    return (lastSlash == std::string::npos) ? filepath : filepath.substr(lastSlash + 1);
}

std::string FileBrowserManager::extractFileExtension(const std::string& filename) {
    size_t lastDot = filename.find_last_of('.');
    return (lastDot == std::string::npos) ? "" : filename.substr(lastDot + 1);
}

std::string FileBrowserManager::getParentDirectory(const std::string& filePath) {
    size_t lastSlash = filePath.find_last_of("/\\");
    return (lastSlash == std::string::npos) ? "/" : filePath.substr(0, lastSlash);
}

std::vector<std::string> FileBrowserManager::getCachedDirectoryElements(const std::string& path) {
    if (cachedDirectoryElements.find(path) != cachedDirectoryElements.end()) {
        return cachedDirectoryElements[path];
    }

    std::vector<std::string> elements = sdService.listElements(path);
    if (elements.size() > 4) {
        if (cachedDirectoryElements.size() >= globalContext.getFileCacheLimit()) {
            cachedDirectoryElements.erase(cachedDirectoryElements.begin());
        }
        cachedDirectoryElements[path] = elements;
    }
    return elements;
}

void FileBrowserManager::removeCachedDirectoryElement(const std::string& path) {
    auto it = cachedDirectoryElements.find(path);
    if (it != cachedDirectoryElements.end()) {
        cachedDirectoryElements.erase(it);
    }
}


} // namespace managers
