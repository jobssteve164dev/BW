#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "Services/BbqrEncoder.h"
#include "Services/SegwitAddressEncoder.h"
#include "Services/TransactionSigningFlow.h"
#include "Services/TransactionReview.h"

using services::BbqrEncoder;
using services::SegwitAddressEncoder;
using services::TransactionOutputReview;
using services::TransactionReview;
using services::TransactionReviewService;
using services::TransactionSigningFlow;
using services::TransactionSigningStage;

namespace {

void testSigningSelectsPsbtBeforeRequestingSecrets() {
    TransactionSigningFlow flow;
    flow.begin();
    assert(flow.stage() == TransactionSigningStage::SELECT_PSBT);
    assert(flow.selectedPsbtPath().empty());
    assert(!flow.secretsLoaded());

    assert(flow.selectPsbt("/payments/outgoing.psbt", false));
    assert(flow.stage() == TransactionSigningStage::UNLOCK_SECRETS);
    assert(flow.selectedPsbtPath() == "/payments/outgoing.psbt");

    assert(flow.secretsLoaded());
    assert(flow.stage() == TransactionSigningStage::REVIEW_AND_SIGN);
    assert(flow.selectedPsbtPath() == "/payments/outgoing.psbt");
}

void testSigningDoesNotAskForSecretsAlreadyLoaded() {
    TransactionSigningFlow flow;
    flow.begin();
    assert(flow.selectPsbt("/ready.psbt", true));
    assert(flow.stage() == TransactionSigningStage::REVIEW_AND_SIGN);
}

void testSigningCancellationClearsTheSelectedTransaction() {
    TransactionSigningFlow flow;
    flow.begin();
    assert(!flow.selectPsbt("", false));
    assert(flow.stage() == TransactionSigningStage::SELECT_PSBT);

    assert(flow.selectPsbt("/cancel.psbt", false));
    flow.cancel();
    assert(flow.stage() == TransactionSigningStage::IDLE);
    assert(flow.selectedPsbtPath().empty());
}

void testSigningRestartForgetsThePreviousPsbt() {
    TransactionSigningFlow flow;
    flow.begin();
    assert(flow.selectPsbt("/payments/first.psbt", false));
    assert(!flow.selectPsbt("/payments/second.psbt", false));

    flow.begin();
    assert(flow.stage() == TransactionSigningStage::SELECT_PSBT);
    assert(flow.selectedPsbtPath().empty());
}

void testReviewComputesFeeFromEveryInputAndOutput() {
    std::vector<TransactionOutputReview> outputs = {
        {125000, "bc1qrecipient"},
        {748500, "bc1qchange"},
    };

    TransactionReview review;
    assert(TransactionReviewService::build(875000, outputs, review));
    assert(review.outputs.size() == 2);
    assert(review.feeSatoshis == 1500);
    assert(TransactionReviewService::formatBitcoin(125000) == "0.00125000 BTC");
    assert(TransactionReviewService::formatBitcoin(100000001) == "1.00000001 BTC");

}

void testReviewRejectsMissingOutputsAndImpossibleFee() {
    TransactionReview review;
    assert(!TransactionReviewService::build(1000, {}, review));

    std::vector<TransactionOutputReview> overspend = {
        {1001, "bc1qrecipient"},
    };
    assert(!TransactionReviewService::build(1000, overspend, review));

    std::vector<TransactionOutputReview> missingAddress = {
        {900, ""},
    };
    assert(!TransactionReviewService::build(1000, missingAddress, review));

    std::vector<TransactionOutputReview> exceedsBitcoinSupply = {
        {2100000000000000ULL, "bc1qfirst"},
        {1, "bc1qsecond"},
    };
    assert(!TransactionReviewService::build(
        2100000000000001ULL,
        exceedsBitcoinSupply,
        review));
}

void testReviewOnlyAcceptsSignaturesThatCommitToTheWholeTransaction() {
    assert(TransactionReviewService::isSupportedSighash(0x00));
    assert(TransactionReviewService::isSupportedSighash(0x01));
    assert(!TransactionReviewService::isSupportedSighash(0x02));
    assert(!TransactionReviewService::isSupportedSighash(0x03));
    assert(!TransactionReviewService::isSupportedSighash(0x81));
}

void testInputAmountRequiresAProvenNativeSegwitPrevout() {
    const std::vector<uint8_t> p2wpkh = {
        0x00, 0x14,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
        0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
    };
    const std::vector<uint8_t> p2pkh = {
        0x76, 0xa9, 0x14,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
        0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
        0x88, 0xac,
    };

    uint64_t amount = 0;
    services::TransactionInputEvidence witnessOnly;
    witnessOnly.hasWitnessUtxo = true;
    witnessOnly.witnessSatoshis = 125000;
    witnessOnly.witnessScript = p2wpkh;
    assert(TransactionReviewService::selectVerifiedInputAmount(witnessOnly, amount));
    assert(amount == 125000);

    services::TransactionInputEvidence fakeLegacyWitness = witnessOnly;
    fakeLegacyWitness.witnessScript = p2pkh;
    assert(!TransactionReviewService::selectVerifiedInputAmount(fakeLegacyWitness, amount));

    services::TransactionInputEvidence unboundPrevious;
    unboundPrevious.hasNonWitnessUtxo = true;
    unboundPrevious.nonWitnessSatoshis = 125000;
    unboundPrevious.nonWitnessScript = p2wpkh;
    assert(!TransactionReviewService::selectVerifiedInputAmount(unboundPrevious, amount));

    services::TransactionInputEvidence conflicting = witnessOnly;
    conflicting.hasNonWitnessUtxo = true;
    conflicting.nonWitnessTxidMatches = true;
    conflicting.nonWitnessSatoshis = 125001;
    conflicting.nonWitnessScript = p2wpkh;
    assert(!TransactionReviewService::selectVerifiedInputAmount(conflicting, amount));

    conflicting.nonWitnessSatoshis = witnessOnly.witnessSatoshis;
    conflicting.nonWitnessScript.back() ^= 0x01;
    assert(!TransactionReviewService::selectVerifiedInputAmount(conflicting, amount));

    conflicting.nonWitnessScript = p2wpkh;
    assert(TransactionReviewService::selectVerifiedInputAmount(conflicting, amount));
    assert(amount == witnessOnly.witnessSatoshis);
}

void testOnlyWalletBip84InputPathsAreAccepted() {
    assert(TransactionReviewService::isSupportedBip84Path({
        0x80000054U, 0x80000000U, 0x80000000U, 0U, 42U,
    }));
    assert(TransactionReviewService::isSupportedBip84Path({
        0x80000054U, 0x80000000U, 0x80000000U, 1U, 0U,
    }));
    assert(!TransactionReviewService::isSupportedBip84Path({
        0x8000002cU, 0x80000000U, 0x80000000U, 0U, 42U,
    }));
    assert(!TransactionReviewService::isSupportedBip84Path({
        0x80000054U, 0x80000000U, 0x80000000U, 2U, 0U,
    }));
    assert(!TransactionReviewService::isSupportedBip84Path({
        0x80000054U, 0x80000000U, 0x80000000U, 0U,
    }));
}

void testBbqrEncodesACompletePsbtInBalancedUppercaseHexFrames() {
    std::vector<uint8_t> psbt(1030);
    for (size_t index = 0; index < psbt.size(); ++index) {
        psbt[index] = static_cast<uint8_t>(index & 0xff);
    }

    std::vector<std::string> frames;
    assert(BbqrEncoder::encodePsbt(psbt, frames, 512));
    assert(frames.size() == 3);
    assert(frames[0].substr(0, 8) == "B$HP0300");
    assert(frames[1].substr(0, 8) == "B$HP0301");
    assert(frames[2].substr(0, 8) == "B$HP0302");
    assert(frames[0].size() == frames[1].size());
    assert(frames[2].size() <= frames[1].size());

    std::string encoded;
    for (const auto& frame : frames) {
        encoded += frame.substr(8);
    }
    assert(encoded.size() == psbt.size() * 2);
    assert(encoded.substr(0, 12) == "000102030405");
    assert(encoded.substr(encoded.size() - 4) == "0405");

    std::vector<std::string> screenFrames;
    assert(BbqrEncoder::encodePsbt(psbt, screenFrames));
    assert(screenFrames.size() > frames.size());
    for (const auto& frame : screenFrames) {
        assert(frame.size() <= 208);
    }
}

void testBbqrRejectsEmptyAndUnrepresentablePayloads() {
    std::vector<std::string> frames;
    assert(!BbqrEncoder::encodePsbt({}, frames, 512));
    assert(!BbqrEncoder::encodePsbt({0x70, 0x73, 0x62, 0x74}, frames, 0));

    std::vector<uint8_t> tooManyParts(1296, 0x42);
    assert(!BbqrEncoder::encodePsbt(tooManyParts, frames, 1));
}

void testSegwitAddressEncodingCoversBech32AndBech32m() {
    const std::vector<uint8_t> versionZeroProgram = {
        0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4, 0x54, 0x94,
        0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23, 0xf1, 0x43, 0x3b, 0xd6,
    };
    assert(SegwitAddressEncoder::encode("bc", 0, versionZeroProgram) ==
           "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4");

    const std::vector<uint8_t> taprootProgram = {
        0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac,
        0x55, 0xa0, 0x62, 0x95, 0xce, 0x87, 0x0b, 0x07,
        0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9,
        0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98,
    };
    assert(SegwitAddressEncoder::encode("bc", 1, taprootProgram) ==
           "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0");
    assert(SegwitAddressEncoder::encode("bc", 0, {0x01}).empty());
    assert(SegwitAddressEncoder::encode("bc", 17, taprootProgram).empty());
}

} // namespace

int main() {
    testSigningSelectsPsbtBeforeRequestingSecrets();
    testSigningDoesNotAskForSecretsAlreadyLoaded();
    testSigningCancellationClearsTheSelectedTransaction();
    testSigningRestartForgetsThePreviousPsbt();
    testReviewComputesFeeFromEveryInputAndOutput();
    testReviewRejectsMissingOutputsAndImpossibleFee();
    testReviewOnlyAcceptsSignaturesThatCommitToTheWholeTransaction();
    testInputAmountRequiresAProvenNativeSegwitPrevout();
    testOnlyWalletBip84InputPathsAreAccepted();
    testBbqrEncodesACompletePsbtInBalancedUppercaseHexFrames();
    testBbqrRejectsEmptyAndUnrepresentablePayloads();
    testSegwitAddressEncodingCoversBech32AndBech32m();
    std::cout << "transaction signing flow tests passed\n";
    return 0;
}
