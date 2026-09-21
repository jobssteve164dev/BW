#ifndef RFID_BACKUP_CODEC_H
#define RFID_BACKUP_CODEC_H

#include <cstdint>
#include <string>
#include <vector>

#include "RfidBackupFormat.h"

namespace services {

struct RfidEncryptedBackup {
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> salt;
    std::vector<uint8_t> tag;
    uint8_t metadata = 0;
};

namespace rfid_backup_detail {

inline void clearBytes(std::vector<uint8_t>& value) {
    volatile uint8_t* data = value.empty() ? nullptr : value.data();
    for (size_t index = 0; index < value.size(); ++index) {
        data[index] = 0;
    }
    value.clear();
}

class SecretBytes {
public:
    ~SecretBytes() { clearBytes(value); }
    std::vector<uint8_t> value;
};

} // namespace rfid_backup_detail

class RfidBackupCodec {
public:
    template <typename Crypto>
    static bool encrypt(Crypto& crypto,
                        const std::vector<uint8_t>& seed,
                        const std::string& password,
                        RfidEncryptedBackup& output) {
        output = {};
        const uint8_t marker = RfidBackupFormat::encryptedMarker(seed.size());
        if (marker == 0 || password.size() < 12) {
            return false;
        }

        try {
            output.metadata = marker;
            output.salt = crypto.generateRandomEsp32(RfidBackupFormat::SALT_SIZE);
            const auto nonce = RfidBackupFormat::nonceFromSalt(output.salt);
            const auto aad = RfidBackupFormat::aad(marker);
            rfid_backup_detail::SecretBytes key;
            key.value = crypto.deriveKeyFromPassphrase(
                password, output.salt, RfidBackupFormat::KDF_ITERATIONS, 32);
            if (output.salt.size() != RfidBackupFormat::SALT_SIZE ||
                nonce.size() != RfidBackupFormat::NONCE_SIZE ||
                key.value.size() != 32 ||
                !crypto.encryptAesGcm(
                    seed, key.value, nonce, aad, output.ciphertext, output.tag) ||
                output.ciphertext.size() != seed.size() ||
                output.tag.size() != RfidBackupFormat::TAG_SIZE) {
                output = {};
                return false;
            }
            return true;
        } catch (...) {
            output = {};
            return false;
        }
    }

    template <typename Crypto>
    static bool decrypt(Crypto& crypto,
                        const RfidEncryptedBackup& input,
                        const std::string& password,
                        std::vector<uint8_t>& seed) {
        rfid_backup_detail::clearBytes(seed);
        const size_t expectedSize = RfidBackupFormat::seedLength(input.metadata);
        if (!RfidBackupFormat::isAuthenticated(input.metadata) ||
            expectedSize == 0 || input.ciphertext.size() != expectedSize ||
            input.salt.size() != RfidBackupFormat::SALT_SIZE ||
            input.tag.size() != RfidBackupFormat::TAG_SIZE || password.empty()) {
            return false;
        }

        try {
            const auto nonce = RfidBackupFormat::nonceFromSalt(input.salt);
            const auto aad = RfidBackupFormat::aad(input.metadata);
            rfid_backup_detail::SecretBytes key;
            key.value = crypto.deriveKeyFromPassphrase(
                password, input.salt, RfidBackupFormat::KDF_ITERATIONS, 32);
            if (nonce.size() != RfidBackupFormat::NONCE_SIZE || key.value.size() != 32 ||
                !crypto.decryptAesGcm(
                    input.ciphertext, key.value, nonce, aad, input.tag, seed) ||
                seed.size() != expectedSize) {
                rfid_backup_detail::clearBytes(seed);
                return false;
            }
            return true;
        } catch (...) {
            rfid_backup_detail::clearBytes(seed);
            return false;
        }
    }
};

} // namespace services

#endif // RFID_BACKUP_CODEC_H
