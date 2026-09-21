#include "RfidBackupFormat.h"

namespace services {

constexpr uint8_t RfidBackupFormat::LEGACY_16;
constexpr uint8_t RfidBackupFormat::LEGACY_32;
constexpr uint8_t RfidBackupFormat::AUTHENTICATED_16;
constexpr uint8_t RfidBackupFormat::AUTHENTICATED_32;
constexpr size_t RfidBackupFormat::SALT_SIZE;
constexpr size_t RfidBackupFormat::NONCE_SIZE;
constexpr size_t RfidBackupFormat::TAG_SIZE;
constexpr uint32_t RfidBackupFormat::KDF_ITERATIONS;

uint8_t RfidBackupFormat::encryptedMarker(size_t seedLength) {
    if (seedLength == 16) {
        return AUTHENTICATED_16;
    }
    if (seedLength == 32) {
        return AUTHENTICATED_32;
    }
    return 0;
}

size_t RfidBackupFormat::seedLength(uint8_t marker) {
    if (marker == LEGACY_16 || marker == AUTHENTICATED_16) {
        return 16;
    }
    if (marker == LEGACY_32 || marker == AUTHENTICATED_32) {
        return 32;
    }
    return 0;
}

bool RfidBackupFormat::isAuthenticated(uint8_t marker) {
    return marker == AUTHENTICATED_16 || marker == AUTHENTICATED_32;
}

std::vector<uint8_t> RfidBackupFormat::nonceFromSalt(const std::vector<uint8_t>& salt) {
    if (salt.size() != SALT_SIZE) {
        return {};
    }
    return std::vector<uint8_t>(salt.begin(), salt.begin() + NONCE_SIZE);
}

std::vector<uint8_t> RfidBackupFormat::aad(uint8_t marker) {
    if (!isAuthenticated(marker)) {
        return {};
    }
    return {'B', 'W', 'R', '2', marker};
}

} // namespace services
