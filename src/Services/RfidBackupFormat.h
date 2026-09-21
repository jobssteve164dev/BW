#ifndef RFID_BACKUP_FORMAT_H
#define RFID_BACKUP_FORMAT_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace services {

class RfidBackupFormat {
public:
    static constexpr uint8_t LEGACY_16 = 0x10;
    static constexpr uint8_t LEGACY_32 = 0x20;
    static constexpr uint8_t AUTHENTICATED_16 = 0x91;
    static constexpr uint8_t AUTHENTICATED_32 = 0x92;
    static constexpr size_t SALT_SIZE = 16;
    static constexpr size_t NONCE_SIZE = 12;
    static constexpr size_t TAG_SIZE = 16;
    static constexpr uint32_t KDF_ITERATIONS = 600000;

    static uint8_t encryptedMarker(size_t seedLength);
    static size_t seedLength(uint8_t marker);
    static bool isAuthenticated(uint8_t marker);
    static std::vector<uint8_t> nonceFromSalt(const std::vector<uint8_t>& salt);
    static std::vector<uint8_t> aad(uint8_t marker);
};

} // namespace services

#endif // RFID_BACKUP_FORMAT_H
