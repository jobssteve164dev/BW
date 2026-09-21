#ifndef RFID_BACKUP_WRITER_H
#define RFID_BACKUP_WRITER_H

#include <cstdint>
#include <vector>

#include "RfidBackupFormat.h"

namespace services {

enum class RfidWriteStatus {
    OK,
    INVALID_INPUT,
    INVALIDATE_FAILED,
    CIPHERTEXT_FAILED,
    SALT_FAILED,
    TAG_FAILED,
    COMMIT_FAILED,
};

class RfidBackupWriter {
public:
    template <typename Device>
    static RfidWriteStatus write(Device& device,
                                 const std::vector<uint8_t>& ciphertextPart1,
                                 const std::vector<uint8_t>& ciphertextPart2,
                                 const std::vector<uint8_t>& salt,
                                 const std::vector<uint8_t>& tag,
                                 uint8_t marker) {
        const size_t seedLength = RfidBackupFormat::seedLength(marker);
        if (!RfidBackupFormat::isAuthenticated(marker) ||
            ciphertextPart1.size() != 16 ||
            ((seedLength == 16 && !ciphertextPart2.empty()) ||
             (seedLength == 32 && ciphertextPart2.size() != 16)) ||
            salt.size() != RfidBackupFormat::SALT_SIZE ||
            tag.size() != RfidBackupFormat::TAG_SIZE) {
            return RfidWriteStatus::INVALID_INPUT;
        }

        // Invalidate the old record before touching any payload block. A valid
        // marker is written only after every authenticated component verifies.
        if (!device.saveMetadata(0)) {
            return RfidWriteStatus::INVALIDATE_FAILED;
        }
        if (!device.savePrivateKey(ciphertextPart1, ciphertextPart2)) {
            return RfidWriteStatus::CIPHERTEXT_FAILED;
        }
        if (!device.saveSalt(salt)) {
            return RfidWriteStatus::SALT_FAILED;
        }
        if (!device.saveChecksum(tag)) {
            return RfidWriteStatus::TAG_FAILED;
        }
        if (!device.saveMetadata(marker)) {
            return RfidWriteStatus::COMMIT_FAILED;
        }
        return RfidWriteStatus::OK;
    }
};

} // namespace services

#endif // RFID_BACKUP_WRITER_H
