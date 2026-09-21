#ifndef FILE_LOAD_ROUTING_H
#define FILE_LOAD_ROUTING_H

#include <string>
#include <vector>

#include <Enums/FileTypeEnum.h>

namespace services {

enum class FileLoadDestination {
    INDEXED_WALLET,
    FILE_BROWSER,
};

struct IndexedWalletLoadResult {
    bool loaded = false;
    bool pathSaved = false;
    std::string path;
};

class FileLoadRouting {
public:
    static FileLoadDestination destination(enums::FileTypeEnum fileType) {
        return fileType == enums::FileTypeEnum::WALLET
            ? FileLoadDestination::INDEXED_WALLET
            : FileLoadDestination::FILE_BROWSER;
    }

    template <typename IndexedWalletAction, typename FileBrowserAction>
    static FileLoadDestination dispatch(
        enums::FileTypeEnum fileType,
        IndexedWalletAction indexedWalletAction,
        FileBrowserAction fileBrowserAction) {
        const auto selectedDestination = destination(fileType);
        if (selectedDestination == FileLoadDestination::INDEXED_WALLET) {
            indexedWalletAction();
        } else {
            fileBrowserAction();
        }
        return selectedDestination;
    }

    static std::vector<std::string> walletCandidates(
        const std::string& savedPath,
        const std::string& defaultPath) {
        std::vector<std::string> paths;
        if (!savedPath.empty()) {
            paths.push_back(savedPath);
        }
        if (!defaultPath.empty() && defaultPath != savedPath) {
            paths.push_back(defaultPath);
        }
        return paths;
    }

    template <typename LoadWallet, typename SavePath>
    static IndexedWalletLoadResult loadFirstValidWallet(
        const std::string& savedPath,
        const std::string& defaultPath,
        LoadWallet loadWallet,
        SavePath savePath) {
        IndexedWalletLoadResult result;
        for (const auto& candidatePath : walletCandidates(savedPath, defaultPath)) {
            if (!loadWallet(candidatePath)) {
                continue;
            }
            result.loaded = true;
            result.path = candidatePath;
            result.pathSaved = candidatePath == savedPath || savePath(candidatePath);
            break;
        }
        return result;
    }
};

} // namespace services

#endif // FILE_LOAD_ROUTING_H
