#ifndef WALLET_H
#define WALLET_H

#include <string>
#include <vector>
#include <cstdint>

namespace models {

class Wallet {
private:
    std::string name;                // Nom du wallet
    std::string zPub;                // Clé publique étendue SegWit (zpub)
    std::string xPub;                // Clé publique étendue Legacy (xpub)
    std::string address;             // Adresse Bitcoin
    std::vector<uint8_t> privateKey; // Only stored if restored or loaded
    std::string mnemonic;            // Only store if restore from SD
    std::string passphrase;          // Only store for signing transaction
    bool secretsLoaded = false;      // Empty BIP39 passphrases are valid loaded values
    std::string fingerprint;         // Master public fingerprint
    std::string derivePath;          // Derivation

public:
    Wallet(const Wallet&) = default;
    Wallet& operator=(const Wallet&) = default;
    Wallet(Wallet&&) noexcept = default;
    Wallet& operator=(Wallet&&) noexcept = default;
    ~Wallet() {
        clearSecrets();
    }

    Wallet() = default;

    Wallet(const std::string& walletName, 
           const std::string& zpubKey, 
           const std::string& addr)
        : name(walletName), zPub(zpubKey), address(addr) {}

    Wallet(const std::string& walletName, 
           const std::string& zpubKey, 
           const std::string& addr,
           const std::string& mnemonic)
        : name(walletName), zPub(zpubKey), address(addr), mnemonic(mnemonic) {}

    Wallet(const std::string& walletName, 
           const std::string& zpubKey, 
           const std::string& addr,
           const std::string& fingerprint,
           const std::string& derivePath)
        : name(walletName), zPub(zpubKey), address(addr),
          fingerprint(fingerprint), derivePath(derivePath) {}

    Wallet(const std::string& walletName,
           const std::string& zpubKey,
           const std::string& addr,
           const std::string& mnemonic,
           const std::string& fingerprint,
           const std::string& derivePath)
        : name(walletName), zPub(zpubKey), address(addr), mnemonic(mnemonic),
          fingerprint(fingerprint), derivePath(derivePath) {}

    // Getters
    std::string getName() const {
        return name;
    }

    std::string getZPub() const {
        return zPub;
    }

    std::string getXPub() const {
        return xPub;
    }

    std::string getAddress() const {
        return address;
    }

    std::vector<uint8_t> getPrivateKey() const {
        return privateKey;
    }

    const std::string& getMnemonic() const {
        return mnemonic;
    }

    const std::string& getPassphrase() const {
        return passphrase;
    }

    bool hasLoadedSecrets() const {
        return secretsLoaded;
    }

    std::string getFingerprint() const {
        return fingerprint;
    }

    std::string getDerivePath() const {
        return derivePath;
    }

    // Setters
    void setName(const std::string& walletName) {
        name = walletName;
    }

    void setZPub(const std::string& zpubKey) {
        zPub = zpubKey;
    }

    void setXPub(const std::string& xpubKey) {
        xPub = xpubKey;
    }

    void setAddress(const std::string& addr) {
        address = addr;
    }

    void setPrivateKey(const std::vector<uint8_t>& priKey) {
        privateKey = priKey;
    }

    void setMnemonic(const std::string& mne) {
        mnemonic = mne;
        secretsLoaded = !mne.empty();
    }

    void setPassphrase(const std::string& pp) {
        passphrase = pp;
    }

    void setFingerprint(const std::string& fp) {
        fingerprint = fp;
    }

    void setDerivePath(const std::string& path) {
        derivePath = path;
    }

    void clearSecrets() {
        volatile char* mnemonicData = mnemonic.empty() ? nullptr : &mnemonic[0];
        for (size_t index = 0; index < mnemonic.size(); ++index) {
            mnemonicData[index] = 0;
        }
        volatile char* passphraseData = passphrase.empty() ? nullptr : &passphrase[0];
        for (size_t index = 0; index < passphrase.size(); ++index) {
            passphraseData[index] = 0;
        }
        volatile uint8_t* privateKeyData = privateKey.empty() ? nullptr : privateKey.data();
        for (size_t index = 0; index < privateKey.size(); ++index) {
            privateKeyData[index] = 0;
        }
        mnemonic.clear();
        passphrase.clear();
        privateKey.clear();
        secretsLoaded = false;
    }

    // Utils
    bool empty() const {
        return name.empty() && zPub.empty() && address.empty();
    }
};

} // namespace models

#endif // WALLET_H
