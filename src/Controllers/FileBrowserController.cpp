#include "FileBrowserController.h"
#include <Services/WalletFileScope.h>

namespace controllers {

FileBrowserController::FileBrowserController(FileBrowserManager& fileBrowserManager)
    : manager(fileBrowserManager) 
{}

void FileBrowserController::handleFileSelection() {
    std::vector<std::string> elementNames;
    std::string fileContent;
    auto selectedFileType = selectionContext.getCurrentSelectedFileType();
    auto& signingFlow = selectionContext.getTransactionSigningFlow();
    std::string transactionRoot;

    if (selectedFileType == FileTypeEnum::TRANSACTION) {
        transactionRoot = services::WalletFileScope::directory(
            selectionContext.getCurrentSelectedWallet().getFingerprint());
        if (transactionRoot.empty()) {
            manager.display.displaySubMessage("钱包标识无效", 50, 1800);
            manager.endTransactionSigning();
            selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
            currentPath = "/";
            return;
        }
    }

    if (selectedFileType == FileTypeEnum::TRANSACTION &&
        signingFlow.stage() == services::TransactionSigningStage::REVIEW_AND_SIGN &&
        !signingFlow.selectedPsbtPath().empty()) {
        currentPath = signingFlow.selectedPsbtPath();
    } else if (selectedFileType == FileTypeEnum::TRANSACTION) {
        currentPath = transactionRoot;
    }

    // Check SD card
    manager.display.displaySubMessage("正在加载", 83);
    manager.sdService.begin(); // SD card start
    if (!manager.sdService.getSdState()) {
        manager.display.displaySubMessage("未找到 SD 卡", 38, 2000);
        manager.sdService.close(); // SD card stop
        if (selectionContext.getTransactionOngoing()) {
            manager.endTransactionSigning();
        }
        manager.selectionContext.setIsModeSelected(false);
        currentPath = "/";
        return;
    }

    if (selectedFileType == FileTypeEnum::TRANSACTION &&
        !manager.sdService.ensureDirectory(transactionRoot)) {
        manager.display.displaySubMessage("无法创建钱包交易目录", 18, 2200);
        manager.sdService.close();
        manager.endTransactionSigning();
        selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
        currentPath = "/";
        return;
    }

    if (selectedFileType == FileTypeEnum::TRANSACTION &&
        signingFlow.stage() == services::TransactionSigningStage::REVIEW_AND_SIGN &&
        (!services::WalletFileScope::contains(transactionRoot, currentPath) ||
         !manager.sdService.isFile(currentPath))) {
        manager.display.displaySubMessage("已选 PSBT 不再可用", 25, 2200);
        manager.sdService.close();
        manager.endTransactionSigning();
        selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
        currentPath = "/";
        return;
    }

    // Get the file content if currentPath is a file or recursively go through folders
    do {
        // currentPath is a file
        if (manager.sdService.isFile(currentPath)) {
            // Load file with correct type
            if(manager.loadFile(currentPath, selectedFileType)) {
                manager.sdService.close(); // succesfully loaded, close sd
                currentPath = "/";
                return;
            }
           
            // Revert to parent dir, currentPath was not a valid file
            currentPath = manager.getParentDirectory(currentPath);
            currentPath = currentPath.empty() ? "/" : currentPath;
        }

        // Get currentPath folder elements from the cache if exist or get them from the sd card
        manager.display.displaySubMessage("正在加载", 83);
        elementNames = selectedFileType == FileTypeEnum::TRANSACTION
            ? manager.sdService.listElements(currentPath, 0, "psbt")
            : manager.getCachedDirectoryElements(currentPath);
        if (elementNames.empty()) {
            if (selectedFileType == FileTypeEnum::TRANSACTION) {
                if (currentPath != transactionRoot) {
                    manager.display.displaySubMessage("此文件夹没有 PSBT", 28, 1500);
                    currentPath = manager.getParentDirectory(currentPath);
                    continue;
                }
                manager.display.displaySubMessage("此钱包暂无 PSBT 文件", 23, 2200);
                manager.input.waitPress();
                manager.sdService.close();
                manager.endTransactionSigning();
                selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
                currentPath = "/";
                return;
            }
        }
        
        // Select the file or folder
        uint16_t currentIndex = selectionContext.getCurrentFileIndex();
        const std::string folderTitle =
            selectedFileType == FileTypeEnum::TRANSACTION && currentPath == transactionRoot
                ? selectionContext.getCurrentSelectedWallet().getName()
                : manager.extractFilename(currentPath);
        currentPath = manager.filePathSelection.select(
            elementNames, folderTitle, currentPath, currentIndex, selectedFileType);
        if (selectedFileType == FileTypeEnum::TRANSACTION &&
            !currentPath.empty() &&
            currentPath != transactionRoot &&
            !services::WalletFileScope::contains(transactionRoot, currentPath)) {
            currentPath.clear();
        }

    } while (!currentPath.empty()); // user hits the return button at root path

    manager.sdService.close(); // SD card stop
    currentPath = "/"; // reset to root path

    if (selectionContext.getTransactionOngoing()) {
        manager.endTransactionSigning();
    }
    selectionContext.setIsModeSelected(false); // go back to menu
    selectionContext.setIsWalletSelected(false);
    selectionContext.setCurrentSelectedFileType(FileTypeEnum::WALLET); // default
    
}

} // namespace controllers
