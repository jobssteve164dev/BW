#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "Services/RfidBackupFormat.h"
#include "Services/RfidBackupCodec.h"
#include "Services/RfidBackupWriter.h"

using services::RfidBackupCodec;
using services::RfidEncryptedBackup;
using services::RfidBackupFormat;
using services::RfidBackupWriter;
using services::RfidWriteStatus;

namespace {

class FakeRfidDevice {
public:
    explicit FakeRfidDevice(size_t failingCall = 0) : failingCall(failingCall) {}

    bool saveMetadata(uint8_t value) {
        calls.push_back(value == 0 ? "invalidate" : "commit");
        return succeeds();
    }

    bool savePrivateKey(const std::vector<uint8_t>&, const std::vector<uint8_t>&) {
        calls.push_back("ciphertext");
        return succeeds();
    }

    bool saveSalt(const std::vector<uint8_t>&) {
        calls.push_back("salt");
        return succeeds();
    }

    bool saveChecksum(const std::vector<uint8_t>&) {
        calls.push_back("tag");
        return succeeds();
    }

    std::vector<std::string> calls;

private:
    bool succeeds() {
        ++callCount;
        return failingCall == 0 || callCount != failingCall;
    }

    size_t failingCall;
    size_t callCount = 0;
};

class FakeAuthenticatedCrypto {
public:
    std::vector<uint8_t> generateRandomEsp32(size_t size) {
        std::vector<uint8_t> result(size);
        for (size_t index = 0; index < size; ++index) {
            result[index] = static_cast<uint8_t>(index);
        }
        return result;
    }

    std::vector<uint8_t> deriveKeyFromPassphrase(
        const std::string& password,
        const std::vector<uint8_t>& salt,
        uint32_t iterations,
        size_t size) {
        if (password.empty() || salt.size() != 16 || iterations != 600000 || size != 32) {
            return {};
        }
        std::vector<uint8_t> key(size);
        for (size_t index = 0; index < size; ++index) {
            key[index] = static_cast<uint8_t>(
                password[index % password.size()] ^ salt[index % salt.size()]);
        }
        return key;
    }

    bool encryptAesGcm(const std::vector<uint8_t>& plaintext,
                       const std::vector<uint8_t>& key,
                       const std::vector<uint8_t>& nonce,
                       const std::vector<uint8_t>& aad,
                       std::vector<uint8_t>& ciphertext,
                       std::vector<uint8_t>& tag) {
        ciphertext.resize(plaintext.size());
        for (size_t index = 0; index < plaintext.size(); ++index) {
            ciphertext[index] = plaintext[index] ^ key[index];
        }
        tag = authenticationTag(ciphertext, key, nonce, aad);
        return true;
    }

    bool decryptAesGcm(const std::vector<uint8_t>& ciphertext,
                       const std::vector<uint8_t>& key,
                       const std::vector<uint8_t>& nonce,
                       const std::vector<uint8_t>& aad,
                       const std::vector<uint8_t>& tag,
                       std::vector<uint8_t>& plaintext) {
        if (tag != authenticationTag(ciphertext, key, nonce, aad)) {
            plaintext.clear();
            return false;
        }
        plaintext.resize(ciphertext.size());
        for (size_t index = 0; index < ciphertext.size(); ++index) {
            plaintext[index] = ciphertext[index] ^ key[index];
        }
        return true;
    }

private:
    static std::vector<uint8_t> authenticationTag(
        const std::vector<uint8_t>& ciphertext,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& nonce,
        const std::vector<uint8_t>& aad) {
        std::vector<uint8_t> tag(16, 0);
        size_t offset = 0;
        for (const auto* bytes : {&ciphertext, &key, &nonce, &aad}) {
            for (uint8_t value : *bytes) {
                tag[offset++ % tag.size()] ^= value;
            }
        }
        return tag;
    }
};

void testRfidWriteCommitsMetadataLast() {
    FakeRfidDevice device;
    const auto status = RfidBackupWriter::write(
        device,
        std::vector<uint8_t>(16, 1),
        {},
        std::vector<uint8_t>(16, 2),
        std::vector<uint8_t>(16, 3),
        RfidBackupFormat::AUTHENTICATED_16);
    assert(status == RfidWriteStatus::OK);
    assert(device.calls == std::vector<std::string>({
        "invalidate", "ciphertext", "salt", "tag", "commit"}));
}

void testRfidWriteNeverCommitsAfterAnIntermediateFailure() {
    const std::vector<RfidWriteStatus> expected = {
        RfidWriteStatus::INVALIDATE_FAILED,
        RfidWriteStatus::CIPHERTEXT_FAILED,
        RfidWriteStatus::SALT_FAILED,
        RfidWriteStatus::TAG_FAILED,
        RfidWriteStatus::COMMIT_FAILED,
    };
    for (size_t failingCall = 1; failingCall <= expected.size(); ++failingCall) {
        FakeRfidDevice device(failingCall);
        const auto status = RfidBackupWriter::write(
            device,
            std::vector<uint8_t>(16, 1),
            {},
            std::vector<uint8_t>(16, 2),
            std::vector<uint8_t>(16, 3),
            RfidBackupFormat::AUTHENTICATED_16);
        assert(status == expected[failingCall - 1]);
        if (failingCall < 5) {
            assert(device.calls.back() != "commit");
        }
    }
}

void testRfidWriteRejectsMalformedInputBeforeTouchingCard() {
    FakeRfidDevice device;
    const auto status = RfidBackupWriter::write(
        device,
        std::vector<uint8_t>(15, 1),
        {},
        std::vector<uint8_t>(16, 2),
        std::vector<uint8_t>(16, 3),
        RfidBackupFormat::AUTHENTICATED_16);
    assert(status == RfidWriteStatus::INVALID_INPUT);
    assert(device.calls.empty());
}

void testRfidWriteSupportsMain32ByteSeedPath() {
    FakeRfidDevice device;
    const auto status = RfidBackupWriter::write(
        device,
        std::vector<uint8_t>(16, 1),
        std::vector<uint8_t>(16, 2),
        std::vector<uint8_t>(16, 3),
        std::vector<uint8_t>(16, 4),
        RfidBackupFormat::AUTHENTICATED_32);
    assert(status == RfidWriteStatus::OK);
    assert(device.calls.back() == "commit");
}

void testAuthenticatedCodecCoversBothSeedSizesAndRejectsTampering() {
    FakeAuthenticatedCrypto crypto;
    for (size_t seedSize : {static_cast<size_t>(16), static_cast<size_t>(32)}) {
        const std::vector<uint8_t> seed(seedSize, 0x5a);
        RfidEncryptedBackup backup;
        assert(RfidBackupCodec::encrypt(crypto, seed, "long-password", backup));
        assert(backup.salt[0] == 0);
        assert(backup.metadata == RfidBackupFormat::encryptedMarker(seedSize));

        std::vector<uint8_t> recovered;
        assert(RfidBackupCodec::decrypt(
            crypto, backup, "long-password", recovered));
        assert(recovered == seed);

        assert(!RfidBackupCodec::decrypt(
            crypto, backup, "wrong-password", recovered));
        assert(recovered.empty());

        auto tamperedCiphertext = backup;
        tamperedCiphertext.ciphertext[0] ^= 1;
        assert(!RfidBackupCodec::decrypt(
            crypto, tamperedCiphertext, "long-password", recovered));

        auto tamperedTag = backup;
        tamperedTag.tag[0] ^= 1;
        assert(!RfidBackupCodec::decrypt(
            crypto, tamperedTag, "long-password", recovered));
    }
}

} // namespace

int main() {
    assert(RfidBackupFormat::encryptedMarker(16) == 0x91);
    assert(RfidBackupFormat::encryptedMarker(32) == 0x92);
    assert(RfidBackupFormat::encryptedMarker(24) == 0);

    assert(RfidBackupFormat::seedLength(0x10) == 16);
    assert(RfidBackupFormat::seedLength(0x20) == 32);
    assert(RfidBackupFormat::seedLength(0x91) == 16);
    assert(RfidBackupFormat::seedLength(0x92) == 32);
    assert(RfidBackupFormat::seedLength(0xff) == 0);

    assert(!RfidBackupFormat::isAuthenticated(0x10));
    assert(!RfidBackupFormat::isAuthenticated(0x20));
    assert(RfidBackupFormat::isAuthenticated(0x91));
    assert(RfidBackupFormat::isAuthenticated(0x92));

    const std::vector<uint8_t> salt = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    };
    const auto nonce = RfidBackupFormat::nonceFromSalt(salt);
    assert(nonce == std::vector<uint8_t>({
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    }));
    assert(RfidBackupFormat::nonceFromSalt({0x01}).empty());
    assert(RfidBackupFormat::aad(0x91) ==
           std::vector<uint8_t>({'B', 'W', 'R', '2', 0x91}));

    testRfidWriteCommitsMetadataLast();
    testRfidWriteNeverCommitsAfterAnIntermediateFailure();
    testRfidWriteRejectsMalformedInputBeforeTouchingCard();
    testRfidWriteSupportsMain32ByteSeedPath();
    testAuthenticatedCodecCoversBothSeedSizesAndRejectsTampering();

    std::cout << "rfid backup format tests passed\n";
    return 0;
}
