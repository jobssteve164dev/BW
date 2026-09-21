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
        display.displayTopBar("正在签名", false, false, true);
        display.displaySubMessage("正在加载", 83);

        // Read file
        auto fileContent = sdService.readBinaryFile(currentPath.c_str());

        // Convert and get signature
        auto psbt = cryptoService.convertPSBTBinaryToBase64(fileContent);
        auto signingWallet = selectionContext.getCurrentSelectedWallet();
        auto signedTransactionBytes = manageBitcoinSignature(psbt, signingWallet.getMnemonic());
        signingWallet.clearSecrets();
        
        // Bad sign
        if (signedTransactionBytes.empty()) {
            display.displaySubMessage("签名失败", 60, 2000);
            return false;
        }

        // Sign success, means it's the correct seed for the correct transaction
        display.displaySubMessage("签名成功", 25, 2000);
        
        // SD Save
        auto parent = getParentDirectory(currentPath);
        std::string baseFileName = fileName.substr(0, fileName.find_last_of('.')); // remove ext .psbt
        sdService.writeBinaryFile((parent + "/" + baseFileName + "-signed.psbt").c_str(), signedTransactionBytes);
        sdService.deleteFile(currentPath.c_str()); // delete unsigned file
        removeCachedDirectoryElement(parent); // new sign.psbt in it, remove to refetch
        display.displaySubMessage("签名已保存到 SD 卡", 40, 3000);
        
        // Check if user want to sign another tx
        auto signConfirmation = confirmationSelection.select("继续签名交易？");
        if (!signConfirmation) {
            // Go back to portfolio
            clearLoadedWalletSecrets(selectionContext.getCurrentSelectedWallet());
            selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
            selectionContext.setCurrentSelectedFileType(FileTypeEnum::WALLET);
            selectionContext.setTransactionOngoing(false);
            return true;
        }

        return false;

    } 

    confirmationSelection.select("不支持此文件");
    return false;
}

bool FileBrowserManager::manageSeedLoadingFile(const std::string& currentPath) {
    auto fileName = extractFilename(currentPath);
    auto fileExt = extractFileExtension(fileName);
    std::string passphrase;

    if (fileExt == "txt") {
        auto fileContent = sdService.readFile(currentPath.c_str());
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

            // Go to file browser
            selectionContext.setCurrentSelectedMode(SelectionModeEnum::LOAD_SD);
            selectionContext.setCurrentSelectedFileType(FileTypeEnum::TRANSACTION);
            display.displaySubMessage("选择 PSBT 文件", 50, 3000);

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
        auto fileContent = sdService.readFile(currentPath.c_str());
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
