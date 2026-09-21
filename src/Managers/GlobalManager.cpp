#include "GlobalManager.h"
#include <algorithm>
#include <stdexcept>
#include <tuple>

namespace managers {
namespace {

void clearSecret(std::string& value) {
    volatile char* data = value.empty() ? nullptr : &value[0];
    for (size_t index = 0; index < value.size(); ++index) {
        data[index] = 0;
    }
    value.clear();
}

void clearSecrets(std::vector<std::string>& values) {
    for (auto& value : values) {
        clearSecret(value);
    }
    values.clear();
}

bool isWalletFile(const std::string& content) {
    return content.find("Filetype: Card Wallet") != std::string::npos &&
           content.find("Version: 2") != std::string::npos;
}

} // namespace

GlobalManager::GlobalManager(CardputerView& display,
                             CardputerInput& input,
                             CryptoService& cryptoService,
                             WalletService& walletService,
                             SdService& sdService,
                             RfidService& rfidService,
                             SettingsService& settingsService,
                             VaultService& vaultService,
                             LedService& ledService,
                             UsbService& usbService,
                             MnemonicSelection& mnemonicSelection,
                             MnemonicRestoreSelection& mnemonicRestoreSelection,
                             StringPromptSelection& stringPromptSelection,
                             ConfirmationSelection& confirmationSelection,
                             SeedRestorationSelection& seedRestorationSelection,
                             FilePathSelection& filePathSelection,
                             KeyboardLayoutSelection& keyboardLayoutSelection,
                             WalletSelection& walletSelection,
                             WalletInformationSelection& walletInformationSelection,
                             ValueSelection& valueSelection
)
    : display(display),
      input(input),
      cryptoService(cryptoService),
      walletService(walletService),
      sdService(sdService),
      rfidService(rfidService),
      settingsService(settingsService),
      vaultService(vaultService),
      ledService(ledService),
      usbService(usbService),
      mnemonicSelection(mnemonicSelection),
      mnemonicRestoreSelection(mnemonicRestoreSelection),
      stringPromptSelection(stringPromptSelection),
      confirmationSelection(confirmationSelection),
      seedRestorationSelection(seedRestorationSelection),
      filePathSelection(filePathSelection),
      keyboardLayoutSelection(keyboardLayoutSelection),
      walletSelection(walletSelection),
      walletInformationSelection(walletInformationSelection),
      valueSelection(valueSelection)
{}

GlobalManager::GlobalManager(const GlobalManager& other)
    : display(other.display),
      input(other.input),
      cryptoService(other.cryptoService),
      walletService(other.walletService),
      sdService(other.sdService),
      rfidService(other.rfidService),
      settingsService(other.settingsService),
      vaultService(other.vaultService),
      ledService(other.ledService),
      usbService(other.usbService),
      mnemonicSelection(other.mnemonicSelection),
      mnemonicRestoreSelection(other.mnemonicRestoreSelection),
      stringPromptSelection(other.stringPromptSelection),
      confirmationSelection(other.confirmationSelection),
      seedRestorationSelection(other.seedRestorationSelection),
      filePathSelection(other.filePathSelection),
      keyboardLayoutSelection(other.keyboardLayoutSelection),
      walletSelection(other.walletSelection),
      walletInformationSelection(other.walletInformationSelection),
      valueSelection(other.valueSelection)
{}

bool GlobalManager::manageSdConfirmation() {
    // SD card start
  display.displaySubMessage("正在加载", 83); // sd can take time to response
  sdService.begin(); 

  // Display 'no SD card'
  if (!sdService.getSdState()) {
    auto confirmation = confirmationSelection.select("未找到 SD 卡");
    if (!confirmation) {
      sdService.close(); // SD card stop
      return false;
    }
  }

  // SD card stop
  sdService.close();
  return true;
}

bool GlobalManager::manageSdSave(Wallet wallet) {
  // Wallets filepath
  auto filePath = globalContext.getFileWalletPath();
  auto defaultPath = globalContext.getfileWalletDefaultPath();
  auto finalPath = filePath.empty() ? defaultPath : filePath;

  std::string fileContent;
  if (sdService.getSdState()) {
    const bool existingFile = sdService.isFile(finalPath) ||
                              sdService.isFile(finalPath + ".bak");
    if (existingFile && !loadWalletFileWithBackup(finalPath, fileContent)) {
      display.displaySubMessage("现有钱包文件损坏，未覆盖", 13, 2200);
      return false;
    }
    if (!walletService.addWallet(wallet)) {
      display.displaySubMessage("钱包数量已达上限", 38, 2200);
      return false;
    }
    fileContent = walletService.getWalletsFileContent(); // new content with the new wallet
    const std::vector<uint8_t> fileBytes(fileContent.begin(), fileContent.end());
    const std::string temporaryPath = finalPath + ".tmp";
    const std::string backupPath = finalPath + ".bak";
    const bool saved = sdService.replaceBinaryFile(finalPath.c_str(),
                                                   temporaryPath.c_str(),
                                                   backupPath.c_str(),
                                                   fileBytes);
    if (saved && !settingsService.saveWalletPath(finalPath)) {
      display.displaySubMessage("钱包已保存，启动路径未保存", 13, 2200);
    }
    return saved;
  } else {
      walletService.addWallet(wallet); // added in memory
      return false;
  }
}

bool GlobalManager::loadWalletFileWithBackup(const std::string& path,
                                             std::string& content) {
  content = sdService.readFile(path.c_str());
  if (isWalletFile(content) && walletService.validateWalletsFile(content)) {
    return walletService.loadAllWallets(content);
  }

  const std::string backupPath = path + ".bak";
  auto backupContent = sdService.readFile(backupPath.c_str());
  if (!isWalletFile(backupContent) || !walletService.validateWalletsFile(backupContent)) {
    return false;
  }

  const std::string corruptPath = path + ".bad";
  if (!sdService.promoteBackupFile(path.c_str(), backupPath.c_str(), corruptPath.c_str())) {
    return false;
  }
  content = std::move(backupContent);
  return walletService.loadAllWallets(content);
}

void GlobalManager::initializePersistentState() {
  auto savedPath = settingsService.loadWalletPath();
  auto defaultPath = globalContext.getfileWalletDefaultPath();
  auto selectedPath = savedPath.empty() ? defaultPath : savedPath;

  if (!sdService.begin()) {
    return;
  }

  std::string fileContent;
  bool loaded = loadWalletFileWithBackup(selectedPath, fileContent);
  if (!loaded && selectedPath != defaultPath) {
    selectedPath = defaultPath;
    loaded = loadWalletFileWithBackup(selectedPath, fileContent);
  }

  if (loaded) {
    globalContext.setFileWalletPath(selectedPath);
    if (savedPath.empty()) {
      settingsService.saveWalletPath(selectedPath);
    }
    const auto vaultStatus = vaultService.inspect();
    if (vaultStatus == VaultStatus::INVALID_FORMAT) {
      display.displaySubMessage("钱包已加载，保险库损坏", 20, 2200);
    } else {
      display.displaySubMessage(
          vaultStatus == VaultStatus::OK ? "钱包与加密备份已就绪" : "钱包已自动加载",
          vaultStatus == VaultStatus::OK ? 23 : 45,
          1500);
    }
  }
  sdService.close();
}

bool GlobalManager::manageVaultSave(const std::vector<uint8_t>& entropy,
                                    const std::string& passphrase,
                                    const Wallet& wallet) {
  if (!sdService.getSdState() || entropy.empty() || wallet.getFingerprint().empty()) {
    return false;
  }

  if (!confirmationSelection.select("加密备份到 SD？")) {
    return false;
  }

  std::string password;
  if (vaultService.exists()) {
    password = stringPromptSelection.select("输入保险库密码", 0, true, true, 8);
  } else {
    password = confirmStringsMatch(
        "设置保险库密码", "再次输入密码", "两次输入不一致", 8);
  }
  if (password.empty()) {
    return false;
  }

  VaultRecord record;
  record.fingerprint = wallet.getFingerprint();
  record.zpub = wallet.getZPub();
  record.entropy = entropy;
  record.passphrase = passphrase;
  display.displaySubMessage("正在加密", 63);
  const auto status = vaultService.upsert(password, record);
  clearSecret(password);
  VaultService::clearRecord(record);

  if (status == VaultStatus::OK) {
    display.displaySubMessage("加密备份已保存", 38, 2000);
    return true;
  }
  if (status == VaultStatus::AUTH_FAILED) {
    display.displaySubMessage("保险库密码错误", 38, 2500);
  } else if (status == VaultStatus::INVALID_FORMAT) {
    display.displaySubMessage("保险库文件损坏", 38, 2500);
  } else {
    display.displaySubMessage("加密备份失败", 46, 2500);
  }
  return false;
}

VaultUnlockResult GlobalManager::manageVaultUnlock(Wallet& wallet) {
  if (!sdService.begin()) {
    return VaultUnlockResult::NOT_AVAILABLE;
  }
  if (!vaultService.exists()) {
    sdService.close();
    return VaultUnlockResult::NOT_AVAILABLE;
  }

  auto password = stringPromptSelection.select("输入保险库密码", 0, true, true, 8);
  if (password.empty()) {
    sdService.close();
    return VaultUnlockResult::CANCELLED_OR_FAILED;
  }

  display.displaySubMessage("正在解锁", 63);
  std::vector<VaultRecord> records;
  const auto status = vaultService.load(password, records);
  clearSecret(password);
  if (status != VaultStatus::OK) {
    VaultService::clearRecords(records);
    sdService.close();
    display.displaySubMessage(
        status == VaultStatus::AUTH_FAILED ? "密码错误或文件被篡改" : "保险库读取失败",
        status == VaultStatus::AUTH_FAILED ? 18 : 38,
        2500);
    return VaultUnlockResult::CANCELLED_OR_FAILED;
  }

  auto record = std::find_if(records.begin(), records.end(), [&](const VaultRecord& item) {
    return item.zpub == wallet.getZPub();
  });
  if (record == records.end()) {
    VaultService::clearRecords(records);
    sdService.close();
    display.displaySubMessage("保险库中没有此钱包", 28, 2500);
    return VaultUnlockResult::CANCELLED_OR_FAILED;
  }

  auto mnemonicWords = cryptoService.privateKeyToMnemonic(record->entropy);
  auto mnemonic = cryptoService.mnemonicVectorToString(mnemonicWords);
  if (mnemonic.empty()) {
    clearSecrets(mnemonicWords);
    VaultService::clearRecords(records);
    sdService.close();
    display.displaySubMessage("保险库内容无效", 46, 2500);
    return VaultUnlockResult::CANCELLED_OR_FAILED;
  }

  const auto derivedZpub = cryptoService.deriveZPub(mnemonic, record->passphrase);
  if (derivedZpub.toString().c_str() != wallet.getZPub()) {
    clearSecret(mnemonic);
    clearSecrets(mnemonicWords);
    VaultService::clearRecords(records);
    sdService.close();
    display.displaySubMessage("备份与钱包不匹配", 30, 2500);
    return VaultUnlockResult::CANCELLED_OR_FAILED;
  }

  wallet.setMnemonic(mnemonic);
  wallet.setPassphrase(record->passphrase);
  walletService.updateWallet(wallet);
  selectionContext.setCurrentSelectedWallet(wallet);
  clearSecret(mnemonic);
  clearSecrets(mnemonicWords);
  VaultService::clearRecords(records);
  sdService.close();
  display.displaySubMessage("保险库已解锁", 46, 1500);
  return VaultUnlockResult::UNLOCKED;
}

void GlobalManager::clearLoadedWalletSecrets(Wallet wallet) {
  wallet.clearSecrets();
  walletService.updateWallet(wallet);
  selectionContext.setCurrentSelectedWallet(wallet);
}

std::string GlobalManager::managePassphrase() {
    auto passConfirmation = confirmationSelection.select("添加附加密码？");
    display.displaySubMessage("正在加载", 83);

    std::string passphrase;
    if (passConfirmation) {
        passphrase = confirmStringsMatch("输入附加密码", "再次输入附加密码", "两次输入不一致");
        display.displaySubMessage("附加密码已设置", 48, 2000);
    }
    return passphrase;
}

std::string GlobalManager::confirmStringsMatch(const std::string& prompt1, 
                                               const std::string& prompt2, 
                                               const std::string& mismatchMessage,
                                               size_t minimumLength)
{
    std::string input1, input2;
    do {
        clearSecret(input1);
        clearSecret(input2);
        input1 = stringPromptSelection.select(prompt1, 0, false, true, minimumLength);
        input2 = stringPromptSelection.select(prompt2, 4, false, true, minimumLength);
        if (input1 != input2) {
            display.displaySubMessage(mismatchMessage, 58, 2000);
        }
    } while (input1 != input2);
    clearSecret(input2);
    return input1;
}

std::tuple<std::vector<uint8_t>, std::string> GlobalManager::manageRfidEncryption(std::vector<uint8_t> privateKey) {
    display.displaySubMessage("正在加载", 83, 800); // Add some time to avoid double input
    auto encryptConfirmation = confirmationSelection.select("加密助记词备份？");
    if (!encryptConfirmation) { 
        return {privateKey, ""};
    }

    // Get salt and encrypt key
    auto salt = cryptoService.getRandomString(16); // 16 bytes, not chars
    auto password = confirmStringsMatch("输入密码", "再次输入密码", "两次输入不一致");
    display.displaySubMessage("正在加载", 83);
    auto encryptedKey = cryptoService.encryptPrivateKeyWithPassphrase(privateKey, password, salt);

    return {encryptedKey, salt};
}

std::vector<uint8_t> GlobalManager::manageRfidDecryption() {
    // Get private key, salt and signature
    auto privateKey = rfidService.getPrivateKey();
    if (privateKey.empty()) {
      display.displaySubMessage("读取密钥失败", 38, 1000);
      return {};
    }
    auto salt = rfidService.getSalt();
    auto sign = rfidService.getCheckSum(); 
    ledService.blink(); // to signal RFID reading

    // If salt is empty, the seed is not emcrypted
    auto saltIsEmpty = std::all_of(salt.begin(), salt.end(), [](int value) { return value == 0; });

    bool validation = false;
    while (!validation && privateKey.size() % 16 == 0 && !saltIsEmpty) {
      // Ask password
      auto password = stringPromptSelection.select("输入密码", 8, true, true);
      if (password.empty()) {return {};} // return button

      // Decrypt
      display.displaySubMessage("正在加载", 83);
      auto decryptedKey = cryptoService.decryptPrivateKeyWithPassphrase(privateKey, password, salt);
      auto generatedSign = cryptoService.generateChecksum(decryptedKey, salt);

      // Verify
      validation = sign == generatedSign;
      if(validation) {
        privateKey = decryptedKey;
        display.displaySubMessage("助记词已解密", 48, 2000);
      } else {
        display.displaySubMessage("密码错误", 55, 1500);
      }
    }
    return privateKey;
}

void GlobalManager::manageRfidSave(std::vector<uint8_t> privateKey) {
  // Confirm RFID before showing hardware-specific instructions.
  auto rfidConfirmation = confirmationSelection.select("另存到 RFID？");
  if (!rfidConfirmation) { return; }

  // Display RFID
  display.displaySeedRfid();
  input.waitPress();
  display.displayTopBar("MIFARE 1K", false, false, false);
  
  // Init RFID
  auto initialised = rfidService.initialize();
  if(!initialised) {
     display.displaySubMessage("未检测到 RFID 模块", 48, 2500);
     return;
  }

  // Get salt, key, sign
  std::vector<uint8_t> returnedKey;
  std::string salt;
  std::tie(returnedKey, salt) = manageRfidEncryption(privateKey);
  auto signature = cryptoService.generateChecksum(privateKey, salt);
  auto splittedKey = cryptoService.splitVector(returnedKey); // return {key, {}} for 16 bytes seed
  
  display.displaySubMessage("请放置 RFID 标签", 43);
  const unsigned long timeout = 5000; // 5 seconds
  unsigned long startTime = millis();
  bool eraseConfirmation = false;

  while (true) {
    if (millis() - startTime > timeout) {
        // Ask confirmation to continue each 5 sec
        auto continueProcess = confirmationSelection.select("重试保存助记词？");
        if (!continueProcess) {
            display.displaySubMessage("已取消保存 RFID", 27, 2000);
            break;
        }
        rfidService.reset();
        startTime = millis();
        display.displaySubMessage("请放置 RFID 标签", 43);
    }

    // No tag detected
    if (!rfidService.isCardPresent()) {
        delay(300);
        continue;
    }

    // Tag already contains a seed
    if (!eraseConfirmation) {
      auto metadataByte = rfidService.getMetadata();
      if (metadataByte == 32 || metadataByte == 16) {
        display.displaySubMessage("标签已有助记词", 28, 1500);
        eraseConfirmation = confirmationSelection.select("覆盖此标签？");
        display.displaySubMessage("请放置 RFID 标签", 43);
        rfidService.reset();
        startTime = millis();
        continue;
      }
    }

    // Save private key
    auto privateKeySaved = rfidService.savePrivateKey(splittedKey.first, splittedKey.second);
    if (!privateKeySaved) {
        display.displaySubMessage("保存密钥失败", 36, 1000);
        continue;
    }

    // Save salt with zeros if no encryption
    auto saltSaved = rfidService.saveSalt(salt);
    if (!saltSaved) {
        display.displaySubMessage("保存盐值失败", 34, 1000);
        continue;
    }

    // Save sign as a checksum for data
    auto signSaved = rfidService.saveChecksum(signature);
    if (!signSaved) {
        display.displaySubMessage("保存校验值失败", 34, 1000);
        continue;
    }

    // Save seed length
    auto lengthSaved = rfidService.saveMetadata(privateKey.size());
    if (!lengthSaved) {
        display.displaySubMessage("保存长度失败", 28, 1000);
        continue;
    }

    ledService.blink();
    display.displaySubMessage("助记词已保存", 54, 2500);
    return;
  }
  rfidService.end();
}

std::vector<uint8_t> GlobalManager::manageRfidRead() {
  auto initialised = rfidService.initialize();
  if(!initialised) {
     display.displayTopBar("RFID 错误");
     display.displaySubMessage("未检测到 RFID 模块", 48, 2500);
     return {};
  }

  display.displayTopBar("MIFARE 1K");
  display.displaySubMessage("请放置 RFID 标签", 43);

  std::vector<uint8_t> privateKey;
  const unsigned long timeout = 5000; // 5 seconds
  unsigned long startTime = millis();

  while (true) {
    if (millis() - startTime > timeout) {
        // Ask confirmation to continue each 5 sec
        auto continueProcess = confirmationSelection.select("重试读取标签？");
        if (!continueProcess) {
            display.displaySubMessage("已取消读取 RFID", 30, 2000);
            return privateKey;
        }
        rfidService.reset();
        startTime = millis();
        display.displaySubMessage("请放置 RFID 标签", 43);
    }

    // No tag detected
    if (!rfidService.isCardPresent()) {
        delay(300);
        continue;
    }

    // Return key if not empty
    privateKey = manageRfidDecryption();
    if (!privateKey.empty()) {
      return privateKey;
    }
  }
}

std::vector<uint8_t> GlobalManager::manageBitcoinSignature(const std::string& psbt,
                                                           const std::string& mnemonic) {
    // Get passphrase or ask for it
    const auto selectedWallet = selectionContext.getCurrentSelectedWallet();
    auto passphrase = selectedWallet.getPassphrase();
    if(!selectedWallet.hasLoadedSecrets()) {
      passphrase = managePassphrase();
    }
    
    // Sign
    auto signedTransactionB64 = cryptoService.signBitcoinTransactions(psbt, mnemonic, passphrase);
    clearSecret(passphrase);
    if(signedTransactionB64.empty()) {return {};}

    // Convert
    auto signedTransactionBytes = cryptoService.convertPSBTBase64ToBinary(signedTransactionB64);
    if(signedTransactionBytes.empty()) {return {};}

    return signedTransactionBytes;
}

Wallet GlobalManager::manageBitcoinWalletCreation(std::string mnemonic, std::string passphrase, 
                                                  std::string walletName, bool loadedConfirmation) {
    // Derive public keys and create segwit BTC address
    display.displaySubMessage("正在加载", 83);
    auto zpub = cryptoService.deriveZPub(mnemonic, passphrase);
    auto fingerprint = cryptoService.getFingerprint(mnemonic, passphrase);
    auto derivePath = cryptoService.getSegwitDerivePath();
    auto addressSegwit = cryptoService.generateBitcoinSegwitAddress(zpub);

    if (loadedConfirmation) {
      display.displaySubMessage("助记词已加载", 65, 2000);
    }

    // Create Wallet
    return Wallet(walletName, zpub.toString().c_str(), addressSegwit, 
                  fingerprint, derivePath);
}

} // namespace managers
