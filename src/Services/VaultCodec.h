#ifndef VAULT_CODEC_H
#define VAULT_CODEC_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace services {

struct VaultRecord {
    std::string fingerprint;
    std::string zpub;
    std::vector<uint8_t> entropy;
    std::string passphrase;

    bool operator==(const VaultRecord& other) const {
        return fingerprint == other.fingerprint && zpub == other.zpub && entropy == other.entropy &&
               passphrase == other.passphrase;
    }
};

struct VaultEnvelope {
    uint32_t iterations = 0;
    std::vector<uint8_t> salt;
    std::vector<uint8_t> nonce;
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> tag;

    bool operator==(const VaultEnvelope& other) const {
        return iterations == other.iterations && salt == other.salt &&
               nonce == other.nonce && ciphertext == other.ciphertext && tag == other.tag;
    }
};

class VaultCodec {
public:
    static constexpr uint8_t FORMAT_VERSION = 1;
    static constexpr uint8_t KDF_PBKDF2_SHA256 = 1;
    static constexpr size_t SALT_SIZE = 16;
    static constexpr size_t NONCE_SIZE = 12;
    static constexpr size_t TAG_SIZE = 16;
    static constexpr size_t HEADER_SIZE = 42;
    static constexpr size_t MAX_RECORDS = 100;
    static constexpr size_t MAX_FINGERPRINT_SIZE = 64;
    static constexpr size_t MAX_ZPUB_SIZE = 128;
    static constexpr size_t MAX_PASSPHRASE_SIZE = 128;
    static constexpr size_t MAX_CIPHERTEXT_SIZE = 32768;
    static constexpr uint32_t MIN_KDF_ITERATIONS = 100000;
    static constexpr uint32_t MAX_KDF_ITERATIONS = 1000000;

    static bool encodeRecords(const std::vector<VaultRecord>& records, std::vector<uint8_t>& output);
    static bool decodeRecords(const std::vector<uint8_t>& input, std::vector<VaultRecord>& records);
    static bool encodeEnvelopeHeader(const VaultEnvelope& envelope,
                                     size_t ciphertextSize,
                                     std::vector<uint8_t>& output);
    static bool encodeEnvelope(const VaultEnvelope& envelope, std::vector<uint8_t>& output);
    static bool decodeEnvelope(const std::vector<uint8_t>& input, VaultEnvelope& envelope);
};

} // namespace services

#endif // VAULT_CODEC_H
