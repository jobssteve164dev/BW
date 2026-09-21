#include "FileBrowserController.h"

namespace controllers {

FileBrowserController::FileBrowserController(FileBrowserManager& fileBrowserManager)
    : manager(fileBrowserManager) 
{}

void FileBrowserController::handleFileSelection() {
    std::vector<std::string> elementNames;
    std::string fileContent;
    auto selectedFileType = selectionContext.getCurrentSelectedFileType();
    auto& signingFlow = selectionContext.getTransactionSigningFlow();

    if (selectedFileType == FileTypeEnum::TRANSACTION &&
        signingFlow.stage() == services::TransactionSigningStage::REVIEW_AND_SIGN &&
        !signingFlow.selectedPsbtPath().empty()) {
        currentPath = signingFlow.selectedPsbtPath();
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
        elementNames = manager.getCachedDirectoryElements(currentPath); 
        
        // Select the file or folder
        uint16_t currentIndex = selectionContext.getCurrentFileIndex();
        currentPath = manager.filePathSelection.select(elementNames, manager.extractFilename(currentPath), 
                                                       currentPath, currentIndex, selectedFileType);

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
