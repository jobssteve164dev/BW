#ifndef TEST_CONTROLLER_SELECTION_CONTEXT_H
#define TEST_CONTROLLER_SELECTION_CONTEXT_H

#include "Enums/WalletInformationEnum.h"

#include <string>

namespace models {

class Wallet {
public:
    Wallet() = default;
    Wallet(const std::string& name,
           const std::string& address,
           const std::string& zpub,
           const std::string& fingerprint,
           const std::string& derivePath)
        : name(name), address(address), zpub(zpub), fingerprint(fingerprint), derivePath(derivePath) {}

    bool empty() const { return name.empty(); }
    std::string getName() const { return name; }
    std::string getAddress() const { return address; }
    std::string getZPub() const { return zpub; }
    std::string getFingerprint() const { return fingerprint; }
    std::string getDerivePath() const { return derivePath; }

private:
    std::string name;
    std::string address;
    std::string zpub;
    std::string fingerprint;
    std::string derivePath;
};

} // namespace models

namespace contexts {

class TransactionSigningFlow {
public:
    void begin() { began = true; }
    bool began = false;
};

class SelectionContext {
public:
    static SelectionContext& getInstance() {
        static SelectionContext instance;
        return instance;
    }

    void reset() { *this = SelectionContext(); }
    models::Wallet getCurrentSelectedWallet() const { return wallet; }
    void setCurrentSelectedWallet(const models::Wallet& selected) { wallet = selected; }
    void setIsWalletSelected(bool selected) { walletSelected = selected; }
    void setIsModeSelected(bool selected) { modeSelected = selected; }
    void setCurrentSelectedMode(enums::SelectionModeEnum selected) { mode = selected; }
    void setCurrentSelectedFileType(enums::FileTypeEnum selected) { fileType = selected; }
    void requestIndexedWalletLoad() { requestedIndexedLoad = true; }
    void setTransactionOngoing(bool ongoing) { transactionOngoing = ongoing; }
    TransactionSigningFlow& getTransactionSigningFlow() { return signingFlow; }

private:
    models::Wallet wallet;
    bool walletSelected = false;
    bool modeSelected = false;
    bool requestedIndexedLoad = false;
    bool transactionOngoing = false;
    enums::SelectionModeEnum mode = enums::SelectionModeEnum::LOAD_SD;
    enums::FileTypeEnum fileType = enums::FileTypeEnum::WALLET;
    TransactionSigningFlow signingFlow;
};

} // namespace contexts

#endif // TEST_CONTROLLER_SELECTION_CONTEXT_H
