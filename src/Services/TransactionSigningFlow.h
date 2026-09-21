#ifndef TRANSACTION_SIGNING_FLOW_H
#define TRANSACTION_SIGNING_FLOW_H

#include <string>

namespace services {

enum class TransactionSigningStage {
    IDLE,
    SELECT_PSBT,
    UNLOCK_SECRETS,
    REVIEW_AND_SIGN,
};

class TransactionSigningFlow {
public:
    void begin() {
        selectedPath.clear();
        currentStage = TransactionSigningStage::SELECT_PSBT;
    }

    bool selectPsbt(const std::string& path, bool secretsAlreadyLoaded) {
        if (currentStage != TransactionSigningStage::SELECT_PSBT || path.empty()) {
            return false;
        }
        selectedPath = path;
        currentStage = secretsAlreadyLoaded
            ? TransactionSigningStage::REVIEW_AND_SIGN
            : TransactionSigningStage::UNLOCK_SECRETS;
        return true;
    }

    bool secretsLoaded() {
        if (currentStage != TransactionSigningStage::UNLOCK_SECRETS || selectedPath.empty()) {
            return false;
        }
        currentStage = TransactionSigningStage::REVIEW_AND_SIGN;
        return true;
    }

    void cancel() {
        selectedPath.clear();
        currentStage = TransactionSigningStage::IDLE;
    }

    TransactionSigningStage stage() const { return currentStage; }
    const std::string& selectedPsbtPath() const { return selectedPath; }

private:
    TransactionSigningStage currentStage = TransactionSigningStage::IDLE;
    std::string selectedPath;
};

} // namespace services

#endif // TRANSACTION_SIGNING_FLOW_H
