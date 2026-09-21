#include "WalletRepository.h"
#include <cctype>

namespace repositories {

bool WalletRepository::addWallet(const Wallet& wallet) {
    if (wallets.size() >= globalContext.getMaxAllowedWallet()) {
        return false;
    }
    wallets.push_back(wallet);
    return true;
}

bool WalletRepository::updateWallet(const Wallet& updatedWallet) {
    for (auto& wallet : wallets) {
        if (wallet.getZPub() == updatedWallet.getZPub() && 
            wallet.getName() == updatedWallet.getName()) {
            wallet.clearSecrets();
            wallet = updatedWallet;
            return true;
        }
    }
    return false; 
}

bool WalletRepository::deleteWallet(const std::string& walletName) {
    auto it = std::remove_if(wallets.begin(), wallets.end(), [&walletName](const Wallet& w) {
        return w.getName() == walletName;
    });

    if (it != wallets.end()) {
        wallets.erase(it, wallets.end()); 
        return true;
    }

    return false; // Wallet non trouvé
}

const std::vector<Wallet>& WalletRepository::getWallets() const {
    return wallets;
}

std::vector<std::string> WalletRepository::splitWallets(const std::string& fileContent) {
    std::vector<std::string> walletDataList;
    std::istringstream stream(fileContent);
    std::string line;
    auto readLine = [&](std::string& output) {
        if (!std::getline(stream, output)) {
            output.clear();
            return false;
        }
        if (!output.empty() && output.back() == '\r') {
            output.pop_back();
        }
        return true;
    };

    if (!readLine(line) || line != "Filetype: Card Wallet" ||
        !readLine(line) || line != "Version: 2") {
        return {};
    }

    while (readLine(line) && line.empty()) {}
    size_t expectedIndex = 1;
    const std::vector<std::string> requiredPrefixes = {
        "Name: ", "zPub: ", "BitcoinAddress: ", "Fingerprint: ", "DerivePath: "
    };

    while (!line.empty()) {
        if (expectedIndex > globalContext.getMaxAllowedWallet() ||
            line != "# WALLET " + std::to_string(expectedIndex)) {
            return {};
        }

        std::string currentWalletData;
        for (const auto& prefix : requiredPrefixes) {
            if (!readLine(line) || line.rfind(prefix, 0) != 0 ||
                line.size() == prefix.size()) {
                return {};
            }
            currentWalletData += line + "\n";
        }
        walletDataList.push_back(currentWalletData);
        ++expectedIndex;

        while (readLine(line) && line.empty()) {}
    }
    return walletDataList;
}

Wallet WalletRepository::parseWallet(const std::string& walletData) {
    std::string name;
    std::string zPub;
    std::string xPub;
    std::string address;
    std::string fingerprint;
    std::string derivePath;

    std::istringstream stream(walletData);
    std::string line;

    while (std::getline(stream, line)) {
        auto delimiterPos = line.find(": ");
        if (delimiterPos == std::string::npos) continue;

        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 2);

        // Clean bad chars
        key.erase(0, key.find_first_not_of(" \t\r\n"));
        key.erase(key.find_last_not_of(" \t\r\n") + 1);
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);

        if (key == "Name") {
            name = value;
        } else if (key == "zPub") {
            zPub = value;
        } else if (key == "BitcoinAddress") {
            address = value;
        } else if (key == "Fingerprint") {
            fingerprint = value;
        } else if (key == "DerivePath") {
            derivePath = value;
        }
    }

    return Wallet(name, zPub, address, "", fingerprint, derivePath);
}

bool WalletRepository::parseWallets(const std::string& fileContent,
                                    std::vector<Wallet>& parsedWallets) {
    parsedWallets.clear();
    auto walletSegments = splitWallets(fileContent);
    if (walletSegments.empty()) {
        return false;
    }

    parsedWallets.reserve(walletSegments.size());
    for (const auto& walletData : walletSegments) {
        auto wallet = parseWallet(walletData);
        const auto fingerprint = wallet.getFingerprint();
        if (wallet.getName().empty() || wallet.getZPub().empty() ||
            wallet.getAddress().empty() || wallet.getFingerprint().empty() ||
            wallet.getDerivePath().empty() ||
            wallet.getZPub().rfind("zpub", 0) != 0 ||
            wallet.getAddress().rfind("bc1", 0) != 0 ||
            fingerprint.size() > 8 ||
            !std::all_of(fingerprint.begin(), fingerprint.end(),
                         [](unsigned char value) { return std::isxdigit(value) != 0; }) ||
            wallet.getDerivePath() != "m/84'/0'/0'/") {
            return false;
        }
        parsedWallets.push_back(std::move(wallet));
    }
    return true;
}

bool WalletRepository::validateWalletsFile(const std::string& fileContent) {
    std::vector<Wallet> parsedWallets;
    return parseWallets(fileContent, parsedWallets);
}

bool WalletRepository::loadAllWallets(const std::string& fileContent) {
    std::vector<Wallet> parsedWallets;
    if (!parseWallets(fileContent, parsedWallets)) {
        return false;
    }

    for (auto& wallet : wallets) {
        wallet.clearSecrets();
    }
    wallets = std::move(parsedWallets);
    return true;
}

std::string WalletRepository::getWalletsFileContent() {
    std::ostringstream fileContent;

    // Header
    fileContent << "Filetype: Card Wallet\n";
    fileContent << "Version: 2\n";
    fileContent << "\n";

    // Wallets
    for (size_t i = 0; i < wallets.size(); ++i) {
        const auto& wallet = wallets[i];

        std::string zPub = wallet.getZPub();
        std::string xPub = wallet.getXPub();
        std::string fingerprint = wallet.getFingerprint();
        std::string derivePath = wallet.getDerivePath();

        fileContent << "# WALLET " << (i + 1) << "\n";
        fileContent << "Name: " << wallet.getName().c_str() << "\n";
        fileContent << "zPub: " << zPub.c_str() << "\n";
        fileContent << "BitcoinAddress: " << wallet.getAddress().c_str() << "\n";
        fileContent << "Fingerprint: " << fingerprint.c_str() << "\n";
        fileContent << "DerivePath: " << derivePath.c_str() << "\n";
        fileContent << "\n";
    }

    return fileContent.str();
}

std::string WalletRepository::vectorToHexString(const std::vector<uint8_t>& vec) {
    std::ostringstream oss;
    for (uint8_t byte : vec) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

std::vector<uint8_t> WalletRepository::hexStringToVector(const std::string& hex) {
    std::vector<uint8_t> vec;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        vec.push_back(byte);
    }
    return vec;
}

} // namespace repositories
