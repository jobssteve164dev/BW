#include "SeedManager.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace managers {
namespace {

void clearBytes(std::vector<uint8_t>& value) {
    volatile uint8_t* data = value.empty() ? nullptr : value.data();
    for (size_t index = 0; index < value.size(); ++index) {
        data[index] = 0;
    }
    value.clear();
}

void clearString(std::string& value) {
    volatile char* data = value.empty() ? nullptr : &value[0];
    for (size_t index = 0; index < value.size(); ++index) {
        data[index] = 0;
    }
    value.clear();
}

void clearWords(std::vector<std::string>& words) {
    for (auto& word : words) {
        clearString(word);
    }
    words.clear();
}

} // namespace

SeedManager::SeedManager(const GlobalManager& gm)
    : GlobalManager(gm) // calls GlobalManager's copy constructor
{}

std::vector<uint8_t> SeedManager::managePrivateKey() {
  // Generate private keys and verify randomness
  EntropyContext::getInstance().collect();
  std::vector<uint8_t> privateKey;
  do {
    clearBytes(privateKey);
    privateKey = cryptoService.generatePrivateKey();
  } while (cryptoService.calculateShanonEntropy(privateKey) < 4.9);

  return privateKey;
}

void SeedManager::manageMnemonicRead(std::vector<std::string>& mnemonic) {
  bool mnemonicIsBackedUp = false;
  bool mnemonicVerification = false;
  std::string word;
  uint8_t randomNumber;

  do {
    // Show the 24 words
    display.displayTopBar("抄写助记词", false, false, true);
    mnemonicSelection.select(mnemonic);

    // Verify Backup
    mnemonicVerification = confirmationSelection.select("验证备份？");
    if (mnemonicVerification) {
      // Use the hardware entropy source so the verification word is not predictable.
      const auto randomBytes = cryptoService.generateRandomEsp32(sizeof(uint32_t));
      uint32_t randomValue = 0;
      std::memcpy(&randomValue, randomBytes.data(), sizeof(randomValue));
      randomNumber = static_cast<uint8_t>(randomValue % mnemonic.size());
      // Ask user the correct word for the given index
      display.displayTopBar("验证助记词", false, false, true);
      auto question = "输入第 " + std::to_string(randomNumber + 1) + " 个单词";
      word = stringPromptSelection.select(question, 8);
    }

    // User want to verify the seed but words are different
    if (mnemonicVerification && word != mnemonic[randomNumber]) {
      if (!word.empty()) {display.displaySubMessage("答案错误", 50, 2000);}
    } else {
      display.displaySubMessage("助记词已备份", 37, 2000);
      mnemonicIsBackedUp = true;
    }
  } while (!mnemonicIsBackedUp);
}

std::vector<std::string> SeedManager::manageMnemonicWrite(size_t wordCount) {
    std::vector<std::string> mnemonicWords;
    mnemonicWords.reserve(wordCount);

    display.displaySeedRestorationInfos();
    input.waitPress();
    
    // Get each word
    for (size_t i = 0; i < wordCount; ++i) {
        std::string word = mnemonicRestoreSelection.select(i, wordCount);
        mnemonicWords.push_back(word);
    }

    // Ensure all words are non empty
    bool allWordsFilled = std::all_of(mnemonicWords.begin(), mnemonicWords.end(),
                                     [](const std::string& word) { return !word.empty(); });
    if (!allWordsFilled) {
        return {};
    }

    // Check if valid mnemonic
    auto mnemonicString = cryptoService.mnemonicVectorToString(mnemonicWords);
    auto mnemonicWordList = cryptoService.mnemonicStringToWordList(mnemonicString);
    if (mnemonicString.empty() || !cryptoService.verifyMnemonic(mnemonicWordList)) {
      return {};
    }

    return mnemonicWords;
}

bool SeedManager::manageMnemonicRestore(size_t wordCount) {
    std::vector<std::string> mnemonicWords;
    mnemonicWords.reserve(wordCount);
    
    // Get each word
    auto mnemonic = manageMnemonicWrite(wordCount);
    if (mnemonic.empty()) { // invalid mnemonic will return empty object
      display.displaySubMessage("助记词无效", 41, 2000);
      return false;
    }

    // Convert
    auto mnemonicString = cryptoService.mnemonicVectorToString(mnemonic);
    auto mnemonicWordList = cryptoService.mnemonicStringToWordList(mnemonicString);
    auto privateKey = cryptoService.mnemonicToPrivateKey(mnemonicString);

    // 助记词有效
    display.displayTopBar("恢复助记词", false, false, true, 5);
    display.displaySubMessage("助记词有效", 45, 2000);

    // Passphrase
    auto passphrase = managePassphrase(); // return "" in case user doesn't want passphrase

    // Prompt for a wallet name
    display.displayTopBar("钱包", false, false, true);
    auto walletName = stringPromptSelection.select("输入钱包名称");
    if (walletName.empty()) {
      clearWords(mnemonic);
      clearBytes(privateKey);
      clearString(mnemonicString);
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

    // Display seed save infos
    display.displaySeedEnd(publicWalletSaved, vaultSaved);
    input.waitPress();

    sdService.close(); // SD card stop

    clearWords(mnemonic);
    clearBytes(privateKey);
    clearString(mnemonicString);
    clearString(passphrase);
    
    // Go to portfolio
    selectionContext.setIsWalletSelected(false);
    selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
    return true;
}

bool SeedManager::manageMnemonicLoading(size_t wordCount) {
    // Get the words from user
    auto mnemonic = manageMnemonicWrite(wordCount);
    if (mnemonic.empty()) { // not valid mnemonic will return empty object
        display.displaySubMessage("助记词无效", 41, 2000);
        return false;
    }

    // At this point mnemonic is valid
    display.displayTopBar("加载助记词", false, false, true, 5);
    display.displaySubMessage("助记词有效", 45, 2000);

    // Get Wallet
    auto wallet = selectionContext.getCurrentSelectedWallet();

    // Get seed passphrase
    auto passphrase = managePassphrase();

    // Derive PublicKey to check if seed match
    display.displaySubMessage("正在加载", 83);
    auto mnemonicString = cryptoService.mnemonicVectorToString(mnemonic);
    auto zPub = cryptoService.deriveZPub(mnemonicString, passphrase);
    if (zPub.toString().c_str() != wallet.getZPub()) {
      display.displaySubMessage("助记词与钱包不匹配", 18, 3000);
      endTransactionSigning();
      selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
      clearWords(mnemonic);
      clearString(mnemonicString);
      clearString(passphrase);
      return false;
    }

    display.displaySubMessage("助记词已加载", 65, 2000);

    // Update
    wallet.setPassphrase(passphrase);
    wallet.setMnemonic(mnemonicString);
    selectionContext.setCurrentSelectedWallet(wallet);
    walletService.updateWallet(wallet);

    if (!selectionContext.getTransactionSigningFlow().secretsLoaded()) {
      endTransactionSigning();
      selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
      clearWords(mnemonic);
      clearString(mnemonicString);
      clearString(passphrase);
      return false;
    }

    // Resume the PSBT selected before secret loading.
    selectionContext.setCurrentSelectedMode(SelectionModeEnum::LOAD_SD);
    selectionContext.setCurrentSelectedFileType(FileTypeEnum::TRANSACTION);
    display.displaySubMessage("正在返回已选交易", 35, 1200);

    sdService.close(); // SD card stop

    clearWords(mnemonic);
    clearString(mnemonicString);
    clearString(passphrase);
    return true;
}

bool SeedManager::manageRfidSeedLoading() {
    // Display infos about module
    display.displayPlugRfid();
    input.waitPress();

    // Get the private key from tag
    auto privateKey = manageRfidRead();
    if (privateKey.empty()) {return false;} // user hits return

    // Convert to mnemonic 24 words
    auto mnemonic = cryptoService.privateKeyToMnemonic(privateKey);

    // Bad seed if empty
    if (mnemonic.empty()) {
      clearBytes(privateKey);
      return false;
    }

    // Get Wallet
    auto wallet = selectionContext.getCurrentSelectedWallet();

    // Get seed passphrase
    auto passphrase = managePassphrase();

    // Derive PublicKey to check if seed match
    display.displaySubMessage("正在加载", 83);
    auto mnemonicString = cryptoService.mnemonicVectorToString(mnemonic);
    auto zPub = cryptoService.deriveZPub(mnemonicString, passphrase);
    if (zPub.toString().c_str() != wallet.getZPub()) {
      display.displaySubMessage("助记词与钱包不匹配", 18, 4000);
      endTransactionSigning();
      selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
      clearWords(mnemonic);
      clearBytes(privateKey);
      clearString(mnemonicString);
      clearString(passphrase);
      return false;
    }

    // Valid seed
    display.displaySubMessage("助记词有效", 63, 1000);
    display.displaySubMessage("首个单词：" + mnemonic[0], 49, 3000);

    // Set mnemonic to wallet
    wallet.setPassphrase(passphrase);
    wallet.setMnemonic(mnemonicString);
    selectionContext.setCurrentSelectedWallet(wallet);
    walletService.updateWallet(wallet);

    if (!selectionContext.getTransactionSigningFlow().secretsLoaded()) {
      endTransactionSigning();
      selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
      clearWords(mnemonic);
      clearBytes(privateKey);
      clearString(mnemonicString);
      clearString(passphrase);
      return false;
    }

    // Resume the PSBT selected before secret loading.
    selectionContext.setCurrentSelectedMode(SelectionModeEnum::LOAD_SD);
    selectionContext.setCurrentSelectedFileType(FileTypeEnum::TRANSACTION);
    display.displaySubMessage("正在返回已选交易", 35, 1200);

    clearWords(mnemonic);
    clearBytes(privateKey);
    clearString(mnemonicString);
    clearString(passphrase);
    return true;
}

void SeedManager::manageRfidSeedRestoration() {
    // Display infos about module
    display.displayPlugRfid();
    input.waitPress();

    // Get the private key from tag
    auto privateKey = manageRfidRead();
    if (privateKey.empty()) {return;} // user hits return

    // Convert to mnemonic words
    auto mnemonic = cryptoService.privateKeyToMnemonic(privateKey);

    // Bad seed if empty
    if (!mnemonic.empty()) {
      display.displaySubMessage("助记词有效", 63, 1000);
      display.displaySubMessage("首个单词：" + mnemonic[0], 47, 3000);
    }

    // Passphrase
    auto mnemonicString = cryptoService.mnemonicVectorToString(mnemonic);
    auto passphrase = managePassphrase(); // return "" in case user doesn't want passphrase

    // Prompt for a wallet name
    display.displayTopBar("钱包", false, false, true);
    auto walletName = stringPromptSelection.select("输入钱包名称");
    if (walletName.empty()) {
      clearWords(mnemonic);
      clearBytes(privateKey);
      clearString(mnemonicString);
      clearString(passphrase);
      return;
    }
    auto wallet = manageBitcoinWalletCreation(mnemonicString, passphrase, walletName);

    // Save wallet to SD if any
    display.displaySubMessage("正在加载", 83);
    sdService.begin(); // SD card start
    auto publicWalletSaved = manageSdSave(wallet);
    auto vaultSaved = manageVaultSave(privateKey, passphrase, wallet);

    // Display seed save infos
    display.displaySeedEnd(publicWalletSaved, vaultSaved);
    input.waitPress();

    sdService.close(); // SD card stop

    // Delete seed
    clearWords(mnemonic);
    clearBytes(privateKey);
    clearString(mnemonicString);
    clearString(passphrase);

    // Go to Portfolio
    selectionContext.setIsWalletSelected(false);
    selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
}

void SeedManager::manageNewSeedCreation() {
    // Display the top main bar with the btc icon
    display.displayTopBar("新建助记词", false, false, true, 5);

    // Check if an SD is plugged
    auto confirmRunWithoutSd = manageSdConfirmation();
    if (!confirmRunWithoutSd) {
        selectionContext.setIsModeSelected(false);
        return;
    }

    // Prompt for a wallet name
    auto walletName = stringPromptSelection.select("输入钱包名称");

    // User hits return (empty name) => go back to menu
    if (walletName.empty()) {
        selectionContext.setIsModeSelected(false);
        return;
    }

    // Display seed infos
    display.displaySeedStart();
    input.waitPress();

    // Generate private key and verify randomness
    auto privateKey = managePrivateKey();

    // Create a mnemonic from private key
    auto mnemonic       = cryptoService.privateKeyToMnemonic(privateKey);
    auto mnemonicString = cryptoService.mnemonicVectorToString(mnemonic);

    // Let the user see the 24 words
    manageMnemonicRead(mnemonic);

    // Optional passphrase
    auto passphrase = managePassphrase(); // returns "" if user doesn't want passphrase

    auto wallet = manageBitcoinWalletCreation(mnemonicString, passphrase, walletName);

    // Save wallet to SD if any
    display.displaySubMessage("正在加载", 83);
    sdService.begin();
    auto publicWalletSaved = manageSdSave(wallet);
    auto vaultSaved = manageVaultSave(privateKey, passphrase, wallet);

    // Optional additional RFID backup
    manageRfidSave(privateKey);

    // Display seed save infos
    display.displaySeedEnd(publicWalletSaved, vaultSaved);
    input.waitPress();

    sdService.close(); // stop SD

    // Delete sensitive data
    clearWords(mnemonic);
    clearBytes(privateKey);
    clearString(mnemonicString);
    clearString(passphrase);

    // Go to Portfolio
    selectionContext.setCurrentSelectedMode(SelectionModeEnum::PORTFOLIO);
}

} // namespace managers
