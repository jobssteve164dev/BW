#ifndef TRANSACTION_REVIEW_H
#define TRANSACTION_REVIEW_H

#include <cstdint>
#include <string>
#include <vector>

namespace services {

struct TransactionOutputReview {
    uint64_t satoshis;
    std::string address;
};

struct TransactionReview {
    std::vector<TransactionOutputReview> outputs;
    uint64_t feeSatoshis;
};

struct TransactionInputEvidence {
    bool hasWitnessUtxo = false;
    uint64_t witnessSatoshis = 0;
    std::vector<uint8_t> witnessScript;
    bool hasNonWitnessUtxo = false;
    bool nonWitnessTxidMatches = false;
    uint64_t nonWitnessSatoshis = 0;
    std::vector<uint8_t> nonWitnessScript;
};

class TransactionReviewService {
public:
    static bool build(uint64_t inputSatoshis,
                      const std::vector<TransactionOutputReview>& outputs,
                      TransactionReview& review);
    static bool isSupportedSighash(uint32_t sighashType);
    static bool isSupportedBip84Path(const std::vector<uint32_t>& path);
    static bool selectVerifiedInputAmount(const TransactionInputEvidence& evidence,
                                          uint64_t& satoshis);
    static std::string formatBitcoin(uint64_t satoshis);
};

} // namespace services

#endif // TRANSACTION_REVIEW_H
