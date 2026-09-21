#include "VaultService.h"

#include <algorithm>
#include <exception>

namespace services {

VaultService::VaultService(CryptoService& cryptoService, SdService& sdService)
    : cryptoService(cryptoService), sdService(sdService) {}

bool VaultService::exists() {
    return sdService.isFile(VAULT_PATH) || sdService.isFile(BACKUP_PATH);
}

VaultStatus VaultService::inspect() {
    bool found = false;
    for (const char* path : {VAULT_PATH, BACKUP_PATH}) {
        if (!sdService.isFile(path)) {
            continue;
        }
        found = true;
        VaultEnvelope envelope;
        if (VaultCodec::decodeEnvelope(
                sdService.readBinaryFile(
                    path, VaultCodec::HEADER_SIZE + VaultCodec::MAX_CIPHERTEXT_SIZE +
                              VaultCodec::TAG_SIZE),
                envelope)) {
            return VaultStatus::OK;
        }
    }
    return found ? VaultStatus::INVALID_FORMAT : VaultStatus::NOT_FOUND;
}

VaultStatus VaultService::load(const std::string& password,
                               std::vector<VaultRecord>& records) {
    clearRecords(records);
    loadedFromBackup = false;

    const auto primaryStatus = loadFromPath(VAULT_PATH, password, records);
    if (primaryStatus == VaultStatus::OK) {
        return primaryStatus;
    }

    const auto backupStatus = loadFromPath(BACKUP_PATH, password, records);
    if (backupStatus == VaultStatus::OK) {
        loadedFromBackup = true;
        return backupStatus;
    }
    if (primaryStatus != VaultStatus::NOT_FOUND) {
        return primaryStatus;
    }
    return backupStatus;
}

VaultStatus VaultService::loadFromPath(const char* path,
                                       const std::string& password,
                                       std::vector<VaultRecord>& records) {
    clearRecords(records);
    if (!sdService.isFile(path)) {
        return VaultStatus::NOT_FOUND;
    }

    const auto fileContent = sdService.readBinaryFile(
        path, VaultCodec::HEADER_SIZE + VaultCodec::MAX_CIPHERTEXT_SIZE + VaultCodec::TAG_SIZE);
    VaultEnvelope envelope;
    if (!VaultCodec::decodeEnvelope(fileContent, envelope)) {
        return VaultStatus::INVALID_FORMAT;
    }

    std::vector<uint8_t> aad;
    if (!VaultCodec::encodeEnvelopeHeader(envelope, envelope.ciphertext.size(), aad)) {
        return VaultStatus::INVALID_FORMAT;
    }

    std::vector<uint8_t> key;
    std::vector<uint8_t> plaintext;
    try {
        key = cryptoService.deriveKeyFromPassphrase(
            password, envelope.salt, envelope.iterations, 32);
        if (key.size() != 32 ||
            !cryptoService.decryptAesGcm(envelope.ciphertext, key, envelope.nonce,
                                         aad, envelope.tag, plaintext)) {
            secureClear(key);
            secureClear(plaintext);
            return VaultStatus::AUTH_FAILED;
        }
    } catch (const std::exception&) {
        secureClear(key);
        secureClear(plaintext);
        return VaultStatus::CRYPTO_ERROR;
    }

    const bool decoded = VaultCodec::decodeRecords(plaintext, records);
    secureClear(key);
    secureClear(plaintext);
    if (!decoded) {
        clearRecords(records);
        return VaultStatus::INVALID_FORMAT;
    }
    return VaultStatus::OK;
}

VaultStatus VaultService::upsert(const std::string& password,
                                 const VaultRecord& record) {
    std::vector<VaultRecord> records;
    if (exists()) {
        const auto loadStatus = load(password, records);
        if (loadStatus != VaultStatus::OK) {
            return loadStatus;
        }
        if (loadedFromBackup &&
            !sdService.promoteBackupFile(VAULT_PATH, BACKUP_PATH, CORRUPT_PATH)) {
            clearRecords(records);
            return VaultStatus::IO_ERROR;
        }
        loadedFromBackup = false;
    }

    auto existing = std::find_if(records.begin(), records.end(), [&](const VaultRecord& item) {
        return item.zpub == record.zpub;
    });
    if (existing == records.end()) {
        records.push_back(record);
    } else {
        secureClear(existing->entropy);
        secureClear(existing->passphrase);
        *existing = record;
    }

    std::vector<uint8_t> plaintext;
    if (!VaultCodec::encodeRecords(records, plaintext)) {
        clearRecords(records);
        return VaultStatus::INVALID_FORMAT;
    }

    VaultEnvelope envelope;
    envelope.iterations = KDF_ITERATIONS;
    envelope.salt = cryptoService.generateRandomEsp32(VaultCodec::SALT_SIZE);
    envelope.nonce = cryptoService.generateRandomEsp32(VaultCodec::NONCE_SIZE);

    std::vector<uint8_t> aad;
    std::vector<uint8_t> key;
    std::vector<uint8_t> fileContent;
    try {
        if (!VaultCodec::encodeEnvelopeHeader(envelope, plaintext.size(), aad)) {
            secureClear(plaintext);
            clearRecords(records);
            return VaultStatus::INVALID_FORMAT;
        }
        key = cryptoService.deriveKeyFromPassphrase(
            password, envelope.salt, envelope.iterations, 32);
        if (key.size() != 32 ||
            !cryptoService.encryptAesGcm(plaintext, key, envelope.nonce, aad,
                                         envelope.ciphertext, envelope.tag) ||
            !VaultCodec::encodeEnvelope(envelope, fileContent)) {
            secureClear(key);
            secureClear(plaintext);
            clearRecords(records);
            return VaultStatus::CRYPTO_ERROR;
        }
    } catch (const std::exception&) {
        secureClear(key);
        secureClear(plaintext);
        clearRecords(records);
        return VaultStatus::CRYPTO_ERROR;
    }

    secureClear(key);
    secureClear(plaintext);
    clearRecords(records);
    return sdService.replaceBinaryFile(VAULT_PATH, TEMPORARY_PATH, BACKUP_PATH, fileContent)
        ? VaultStatus::OK
        : VaultStatus::IO_ERROR;
}

void VaultService::clearRecords(std::vector<VaultRecord>& records) {
    for (auto& record : records) {
        clearRecord(record);
    }
    records.clear();
}

void VaultService::clearRecord(VaultRecord& record) {
    secureClear(record.entropy);
    secureClear(record.passphrase);
    secureClear(record.fingerprint);
    secureClear(record.zpub);
}

void VaultService::secureClear(std::vector<uint8_t>& value) {
    volatile uint8_t* current = value.empty() ? nullptr : value.data();
    for (size_t index = 0; index < value.size(); ++index) {
        current[index] = 0;
    }
    value.clear();
}

void VaultService::secureClear(std::string& value) {
    volatile char* current = value.empty() ? nullptr : &value[0];
    for (size_t index = 0; index < value.size(); ++index) {
        current[index] = 0;
    }
    value.clear();
}

} // namespace services
