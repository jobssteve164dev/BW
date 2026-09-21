#include "TransactionReview.h"

#include <iomanip>
#include <limits>
#include <sstream>

namespace services {
namespace {

constexpr uint64_t MAX_BITCOIN_SATOSHIS = 21000000ULL * 100000000ULL;

bool isNativeP2wpkh(const std::vector<uint8_t>& script) {
    return script.size() == 22 && script[0] == 0x00 && script[1] == 0x14;
}

} // namespace

bool TransactionReviewService::build(
    uint64_t inputSatoshis,
    const std::vector<TransactionOutputReview>& outputs,
    TransactionReview& review) {
    review.outputs.clear();
    review.feeSatoshis = 0;
    if (outputs.empty() || inputSatoshis > MAX_BITCOIN_SATOSHIS) {
        return false;
    }

    uint64_t outputSatoshis = 0;
    for (const auto& output : outputs) {
        if (output.address.empty() || output.satoshis > MAX_BITCOIN_SATOSHIS ||
            output.satoshis > std::numeric_limits<uint64_t>::max() - outputSatoshis) {
            return false;
        }
        outputSatoshis += output.satoshis;
        if (outputSatoshis > MAX_BITCOIN_SATOSHIS) {
            return false;
        }
    }
    if (inputSatoshis < outputSatoshis) {
        return false;
    }

    review.outputs = outputs;
    review.feeSatoshis = inputSatoshis - outputSatoshis;
    return true;
}

bool TransactionReviewService::isSupportedSighash(uint32_t sighashType) {
    return sighashType == 0 || sighashType == 1;
}

bool TransactionReviewService::isSupportedBip84Path(const std::vector<uint32_t>& path) {
    return path.size() == 5 &&
           path[0] == 0x80000054U &&
           path[1] == 0x80000000U &&
           path[2] == 0x80000000U &&
           path[3] <= 1U &&
           path[4] < 0x80000000U;
}

bool TransactionReviewService::selectVerifiedInputAmount(
    const TransactionInputEvidence& evidence,
    uint64_t& satoshis) {
    satoshis = 0;
    if (!evidence.hasWitnessUtxo && !evidence.hasNonWitnessUtxo) {
        return false;
    }
    if (evidence.hasNonWitnessUtxo && !evidence.nonWitnessTxidMatches) {
        return false;
    }
    if (evidence.hasWitnessUtxo && evidence.hasNonWitnessUtxo &&
        (evidence.witnessSatoshis != evidence.nonWitnessSatoshis ||
         evidence.witnessScript != evidence.nonWitnessScript)) {
        return false;
    }

    const auto& script = evidence.hasWitnessUtxo
        ? evidence.witnessScript
        : evidence.nonWitnessScript;
    const uint64_t amount = evidence.hasWitnessUtxo
        ? evidence.witnessSatoshis
        : evidence.nonWitnessSatoshis;
    if (!isNativeP2wpkh(script) || amount > MAX_BITCOIN_SATOSHIS) {
        return false;
    }

    satoshis = amount;
    return true;
}

std::string TransactionReviewService::formatBitcoin(uint64_t satoshis) {
    std::ostringstream output;
    output << satoshis / 100000000ULL << '.'
           << std::setw(8) << std::setfill('0') << satoshis % 100000000ULL
           << " BTC";
    return output.str();
}

} // namespace services
