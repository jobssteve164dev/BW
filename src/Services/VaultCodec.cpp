#include "VaultCodec.h"

#include <algorithm>
#include <limits>

namespace services {
namespace {

constexpr uint8_t MAGIC[] = {'B', 'W', 'V', '1'};

void appendUint16(std::vector<uint8_t>& output, uint16_t value) {
    output.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    output.push_back(static_cast<uint8_t>(value & 0xff));
}

void appendUint32(std::vector<uint8_t>& output, uint32_t value) {
    output.push_back(static_cast<uint8_t>((value >> 24) & 0xff));
    output.push_back(static_cast<uint8_t>((value >> 16) & 0xff));
    output.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    output.push_back(static_cast<uint8_t>(value & 0xff));
}

bool readUint8(const std::vector<uint8_t>& input, size_t& offset, uint8_t& value) {
    if (offset >= input.size()) {
        return false;
    }
    value = input[offset++];
    return true;
}

bool readUint16(const std::vector<uint8_t>& input, size_t& offset, uint16_t& value) {
    if (input.size() - offset < 2) {
        return false;
    }
    value = (static_cast<uint16_t>(input[offset]) << 8) |
            static_cast<uint16_t>(input[offset + 1]);
    offset += 2;
    return true;
}

bool readUint32(const std::vector<uint8_t>& input, size_t& offset, uint32_t& value) {
    if (input.size() - offset < 4) {
        return false;
    }
    value = (static_cast<uint32_t>(input[offset]) << 24) |
            (static_cast<uint32_t>(input[offset + 1]) << 16) |
            (static_cast<uint32_t>(input[offset + 2]) << 8) |
            static_cast<uint32_t>(input[offset + 3]);
    offset += 4;
    return true;
}

bool readBytes(const std::vector<uint8_t>& input,
               size_t& offset,
               size_t length,
               std::vector<uint8_t>& output) {
    if (length > input.size() - offset) {
        return false;
    }
    output.assign(input.begin() + offset, input.begin() + offset + length);
    offset += length;
    return true;
}

bool readString(const std::vector<uint8_t>& input,
                size_t& offset,
                size_t length,
                std::string& output) {
    if (length > input.size() - offset) {
        return false;
    }
    output.assign(reinterpret_cast<const char*>(input.data() + offset), length);
    offset += length;
    return true;
}

bool validEntropySize(size_t size) {
    return size == 16 || size == 32;
}

void clearBytes(std::vector<uint8_t>& value) {
    volatile uint8_t* data = value.empty() ? nullptr : value.data();
    for (size_t index = 0; index < value.size(); ++index) {
        data[index] = 0;
    }
    value.clear();
}

void clearString(std::string& value) {
    volatile char* data = value.empty() ? nullptr : &value[0];
    for (size_t index = 0; index < value.size(); ++index) {
        data[index] = 0;
    }
    value.clear();
}

void clearRecord(VaultRecord& record) {
    clearString(record.fingerprint);
    clearString(record.zpub);
    clearBytes(record.entropy);
    clearString(record.passphrase);
}

void clearRecords(std::vector<VaultRecord>& records) {
    for (auto& record : records) {
        clearRecord(record);
    }
    records.clear();
}

} // namespace

bool VaultCodec::encodeRecords(const std::vector<VaultRecord>& records,
                               std::vector<uint8_t>& output) {
    clearBytes(output);
    if (records.empty() || records.size() > MAX_RECORDS) {
        return false;
    }

    appendUint16(output, static_cast<uint16_t>(records.size()));
    for (const auto& record : records) {
        if (record.fingerprint.empty() || record.fingerprint.size() > MAX_FINGERPRINT_SIZE ||
            record.zpub.empty() || record.zpub.size() > MAX_ZPUB_SIZE ||
            !validEntropySize(record.entropy.size()) ||
            record.passphrase.size() > MAX_PASSPHRASE_SIZE) {
            clearBytes(output);
            return false;
        }

        output.push_back(static_cast<uint8_t>(record.fingerprint.size()));
        output.insert(output.end(), record.fingerprint.begin(), record.fingerprint.end());
        output.push_back(static_cast<uint8_t>(record.zpub.size()));
        output.insert(output.end(), record.zpub.begin(), record.zpub.end());
        output.push_back(static_cast<uint8_t>(record.entropy.size()));
        output.insert(output.end(), record.entropy.begin(), record.entropy.end());
        appendUint16(output, static_cast<uint16_t>(record.passphrase.size()));
        output.insert(output.end(), record.passphrase.begin(), record.passphrase.end());
    }
    return true;
}

bool VaultCodec::decodeRecords(const std::vector<uint8_t>& input,
                               std::vector<VaultRecord>& records) {
    clearRecords(records);
    size_t offset = 0;
    uint16_t recordCount = 0;
    if (!readUint16(input, offset, recordCount) || recordCount == 0 || recordCount > MAX_RECORDS) {
        return false;
    }

    records.reserve(recordCount);
    for (uint16_t index = 0; index < recordCount; ++index) {
        records.emplace_back();
        auto& record = records.back();
        uint8_t fingerprintSize = 0;
        uint8_t zpubSize = 0;
        uint8_t entropySize = 0;
        uint16_t passphraseSize = 0;

        if (!readUint8(input, offset, fingerprintSize) || fingerprintSize == 0 ||
            fingerprintSize > MAX_FINGERPRINT_SIZE ||
            !readString(input, offset, fingerprintSize, record.fingerprint) ||
            !readUint8(input, offset, zpubSize) || zpubSize == 0 ||
            zpubSize > MAX_ZPUB_SIZE ||
            !readString(input, offset, zpubSize, record.zpub) ||
            !readUint8(input, offset, entropySize) || !validEntropySize(entropySize) ||
            !readBytes(input, offset, entropySize, record.entropy) ||
            !readUint16(input, offset, passphraseSize) ||
            passphraseSize > MAX_PASSPHRASE_SIZE ||
            !readString(input, offset, passphraseSize, record.passphrase)) {
            clearRecords(records);
            return false;
        }
    }

    if (offset != input.size()) {
        clearRecords(records);
        return false;
    }
    return true;
}

bool VaultCodec::encodeEnvelopeHeader(const VaultEnvelope& envelope,
                                      size_t ciphertextSize,
                                      std::vector<uint8_t>& output) {
    output.clear();
    if (envelope.iterations < MIN_KDF_ITERATIONS ||
        envelope.iterations > MAX_KDF_ITERATIONS ||
        envelope.salt.size() != SALT_SIZE ||
        envelope.nonce.size() != NONCE_SIZE || ciphertextSize == 0 ||
        ciphertextSize > MAX_CIPHERTEXT_SIZE ||
        ciphertextSize > std::numeric_limits<uint32_t>::max()) {
        return false;
    }

    output.insert(output.end(), std::begin(MAGIC), std::end(MAGIC));
    output.push_back(FORMAT_VERSION);
    output.push_back(KDF_PBKDF2_SHA256);
    appendUint32(output, envelope.iterations);
    output.insert(output.end(), envelope.salt.begin(), envelope.salt.end());
    output.insert(output.end(), envelope.nonce.begin(), envelope.nonce.end());
    appendUint32(output, static_cast<uint32_t>(ciphertextSize));
    return output.size() == HEADER_SIZE;
}

bool VaultCodec::encodeEnvelope(const VaultEnvelope& envelope,
                                std::vector<uint8_t>& output) {
    if (envelope.tag.size() != TAG_SIZE ||
        !encodeEnvelopeHeader(envelope, envelope.ciphertext.size(), output)) {
        output.clear();
        return false;
    }
    output.insert(output.end(), envelope.ciphertext.begin(), envelope.ciphertext.end());
    output.insert(output.end(), envelope.tag.begin(), envelope.tag.end());
    return true;
}

bool VaultCodec::decodeEnvelope(const std::vector<uint8_t>& input,
                                VaultEnvelope& envelope) {
    envelope = {};
    if (input.size() < HEADER_SIZE + TAG_SIZE ||
        !std::equal(std::begin(MAGIC), std::end(MAGIC), input.begin()) ||
        input[4] != FORMAT_VERSION || input[5] != KDF_PBKDF2_SHA256) {
        return false;
    }

    size_t offset = 6;
    uint32_t ciphertextSize = 0;
    if (!readUint32(input, offset, envelope.iterations) ||
        envelope.iterations < MIN_KDF_ITERATIONS ||
        envelope.iterations > MAX_KDF_ITERATIONS ||
        !readBytes(input, offset, SALT_SIZE, envelope.salt) ||
        !readBytes(input, offset, NONCE_SIZE, envelope.nonce) ||
        !readUint32(input, offset, ciphertextSize) || ciphertextSize == 0 ||
        ciphertextSize > MAX_CIPHERTEXT_SIZE ||
        input.size() != HEADER_SIZE + static_cast<size_t>(ciphertextSize) + TAG_SIZE ||
        !readBytes(input, offset, ciphertextSize, envelope.ciphertext) ||
        !readBytes(input, offset, TAG_SIZE, envelope.tag)) {
        envelope = {};
        return false;
    }
    return offset == input.size();
}

} // namespace services
