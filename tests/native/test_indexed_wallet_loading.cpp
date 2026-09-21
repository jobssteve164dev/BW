#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "Contexts/SelectionContext.h"
#include "Services/FileLoadRouting.h"

using enums::FileTypeEnum;
using enums::SelectionModeEnum;
using services::FileLoadDestination;
using services::FileLoadRouting;

namespace {

void testWalletLoadingUsesTheIndexedFileInsteadOfTheBrowser() {
    assert(FileLoadRouting::destination(FileTypeEnum::WALLET) ==
           FileLoadDestination::INDEXED_WALLET);
    assert(FileLoadRouting::destination(FileTypeEnum::SEED) ==
           FileLoadDestination::FILE_BROWSER);
    assert(FileLoadRouting::destination(FileTypeEnum::TRANSACTION) ==
           FileLoadDestination::FILE_BROWSER);
}

void testSavedWalletPathFallsBackToTheCanonicalIndexFile() {
    const auto paths = FileLoadRouting::walletCandidates(
        "/wallets/current.txt", "/card-wallets.txt");
    assert(paths == std::vector<std::string>({
        "/wallets/current.txt", "/card-wallets.txt"}));

    const auto defaultOnly = FileLoadRouting::walletCandidates(
        "", "/card-wallets.txt");
    assert(defaultOnly == std::vector<std::string>({"/card-wallets.txt"}));

    const auto noDuplicate = FileLoadRouting::walletCandidates(
        "/card-wallets.txt", "/card-wallets.txt");
    assert(noDuplicate == std::vector<std::string>({"/card-wallets.txt"}));
}

void testMainMenuWalletLoadingOverridesAStaleBrowserType() {
    auto& context = contexts::SelectionContext::getInstance();
    context.setCurrentSelectedFileType(FileTypeEnum::SEED);
    context.setCurrentSelectedMode(SelectionModeEnum::LOAD_SEED);
    context.setIsModeSelected(false);

    context.requestIndexedWalletLoad();

    assert(context.getCurrentSelectedFileType() == FileTypeEnum::WALLET);
    assert(context.getCurrentSelectedMode() == SelectionModeEnum::LOAD_SD);
    assert(context.getIsModeSelected());
}

void testWalletDispatchExecutesOnlyTheIndexedLoader() {
    std::string action;
    const auto walletDestination = FileLoadRouting::dispatch(
        FileTypeEnum::WALLET,
        [&action]() { action = "indexed"; },
        [&action]() { action = "browser"; });
    assert(walletDestination == FileLoadDestination::INDEXED_WALLET);
    assert(action == "indexed");

    action.clear();
    const auto seedDestination = FileLoadRouting::dispatch(
        FileTypeEnum::SEED,
        [&action]() { action = "indexed"; },
        [&action]() { action = "browser"; });
    assert(seedDestination == FileLoadDestination::FILE_BROWSER);
    assert(action == "browser");
}

void testInvalidSavedPathLoadsAndPersistsTheCanonicalIndex() {
    std::vector<std::string> attempts;
    std::vector<std::string> savedPaths;
    const auto result = FileLoadRouting::loadFirstValidWallet(
        "/missing-wallets.txt",
        "/card-wallets.txt",
        [&attempts](const std::string& path) {
            attempts.push_back(path);
            return path == "/card-wallets.txt";
        },
        [&savedPaths](const std::string& path) {
            savedPaths.push_back(path);
            return true;
        });

    assert(attempts == std::vector<std::string>({
        "/missing-wallets.txt", "/card-wallets.txt"}));
    assert(savedPaths == std::vector<std::string>({"/card-wallets.txt"}));
    assert(result.loaded);
    assert(result.pathSaved);
    assert(result.path == "/card-wallets.txt");
}

} // namespace

int main() {
    testWalletLoadingUsesTheIndexedFileInsteadOfTheBrowser();
    testSavedWalletPathFallsBackToTheCanonicalIndexFile();
    testMainMenuWalletLoadingOverridesAStaleBrowserType();
    testWalletDispatchExecutesOnlyTheIndexedLoader();
    testInvalidSavedPathLoadsAndPersistsTheCanonicalIndex();
    std::cout << "indexed wallet loading tests passed\n";
    return 0;
}
