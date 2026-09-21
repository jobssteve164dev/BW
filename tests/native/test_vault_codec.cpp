#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "Services/VaultCodec.h"

using services::VaultCodec;
using services::VaultEnvelope;
using services::VaultRecord;

namespace {

void testRecordsUseStableBinaryFormat() {
    VaultRecord record;
    record.fingerprint = "a1b2";
    record.zpub = "zpub-test";
    record.entropy = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                      0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};
    record.passphrase = "key";

    const std::vector<uint8_t> expected = {
        0x00, 0x01,
        0x04, 'a', '1', 'b', '2',
        0x09, 'z', 'p', 'u', 'b', '-', 't', 'e', 's', 't',
        0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x00, 0x03, 'k', 'e', 'y'
    };

    std::vector<uint8_t> encoded;
    assert(VaultCodec::encodeRecords({record}, encoded));
    assert(encoded == expected);

    std::vector<VaultRecord> decoded;
    assert(VaultCodec::decodeRecords(encoded, decoded));
    assert(decoded.size() == 1);
    assert(decoded[0] == record);
}

void testRecordsRejectTruncationAndInvalidEntropyLength() {
    const std::vector<uint8_t> truncated = {0x00, 0x01, 0x04, 'a'};
    std::vector<VaultRecord> decoded;
    assert(!VaultCodec::decodeRecords(truncated, decoded));

    VaultRecord invalid;
    invalid.fingerprint = "abcd";
    invalid.zpub = "zpub-test";
    invalid.entropy = std::vector<uint8_t>(15, 0x42);
    std::vector<uint8_t> encoded;
    assert(!VaultCodec::encodeRecords({invalid}, encoded));
}

void testEnvelopeRoundTripsAndRejectsTrailingOrWrongMagic() {
    VaultEnvelope envelope;
    envelope.iterations = 200000;
    envelope.salt = std::vector<uint8_t>(16, 0x11);
    envelope.nonce = std::vector<uint8_t>(12, 0x22);
    envelope.ciphertext = {0xaa, 0xbb, 0xcc};
    envelope.tag = std::vector<uint8_t>(16, 0x33);

    std::vector<uint8_t> encoded;
    assert(VaultCodec::encodeEnvelope(envelope, encoded));
    assert(encoded.size() == 61);
    assert(encoded[0] == 'B' && encoded[1] == 'W' && encoded[2] == 'V' && encoded[3] == '1');
    assert(encoded[4] == 1);
    assert(encoded[5] == 1);
    assert(encoded[6] == 0x00 && encoded[7] == 0x03 && encoded[8] == 0x0d && encoded[9] == 0x40);

    VaultEnvelope decoded;
    assert(VaultCodec::decodeEnvelope(encoded, decoded));
    assert(decoded == envelope);

    auto wrongMagic = encoded;
    wrongMagic[0] = 'X';
    assert(!VaultCodec::decodeEnvelope(wrongMagic, decoded));

    auto trailing = encoded;
    trailing.push_back(0x00);
    assert(!VaultCodec::decodeEnvelope(trailing, decoded));
}

void testEnvelopeRejectsUnsafeKdfWorkFactors() {
    VaultEnvelope envelope;
    envelope.iterations = 1;
    envelope.salt = std::vector<uint8_t>(16, 0x11);
    envelope.nonce = std::vector<uint8_t>(12, 0x22);
    envelope.ciphertext = {0xaa};
    envelope.tag = std::vector<uint8_t>(16, 0x33);

    std::vector<uint8_t> encoded;
    assert(!VaultCodec::encodeEnvelope(envelope, encoded));

    envelope.iterations = 1000001;
    assert(!VaultCodec::encodeEnvelope(envelope, encoded));

    envelope.iterations = 200000;
    assert(VaultCodec::encodeEnvelope(envelope, encoded));
    encoded[6] = 0x00;
    encoded[7] = 0x00;
    encoded[8] = 0x00;
    encoded[9] = 0x01;
    VaultEnvelope decoded;
    assert(!VaultCodec::decodeEnvelope(encoded, decoded));
}

} // namespace

int main() {
    testRecordsUseStableBinaryFormat();
    testRecordsRejectTruncationAndInvalidEntropyLength();
    testEnvelopeRoundTripsAndRejectsTrailingOrWrongMagic();
    testEnvelopeRejectsUnsafeKdfWorkFactors();
    std::cout << "vault codec tests passed\n";
    return 0;
}
