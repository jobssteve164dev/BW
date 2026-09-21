#include "CryptoService.h"
#include "mbedtls/sha256.h"
#undef PSTR // conflict
#undef F
#include <cryptopp/ripemd.h>
#include <cstring>
#include <algorithm>
#include <limits>
#include "bootloader_random.h"
#include "esp_random.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/gcm.h"
#include "cryptopp/rng.h"
#include "SegwitAddressEncoder.h"

namespace services {
namespace {

class HardwareEntropyScope {
public:
    HardwareEntropyScope() { bootloader_random_enable(); }
    ~HardwareEntropyScope() { bootloader_random_disable(); }

    HardwareEntropyScope(const HardwareEntropyScope&) = delete;
    HardwareEntropyScope& operator=(const HardwareEntropyScope&) = delete;
};

void secureClear(std::vector<uint8_t>& value) {
    volatile uint8_t* data = value.empty() ? nullptr : value.data();
    for (size_t index = 0; index < value.size(); ++index) {
        data[index] = 0;
    }
    value.clear();
}

void secureClear(std::string& value) {
    volatile char* data = value.empty() ? nullptr : &value[0];
    for (size_t index = 0; index < value.size(); ++index) {
        data[index] = 0;
    }
    value.clear();
}

std::string outputAddress(const wally_tx_output& output) {
    if (!output.script || output.script_len < 4) {
        return "";
    }

    uint8_t witnessVersion = 0xff;
    if (output.script[0] == 0x00) {
        witnessVersion = 0;
    } else if (output.script[0] >= 0x51 && output.script[0] <= 0x60) {
        witnessVersion = output.script[0] - 0x50;
    }
    if (witnessVersion <= 16 && output.script[1] == output.script_len - 2) {
        const std::vector<uint8_t> program(output.script + 2, output.script + output.script_len);
        return SegwitAddressEncoder::encode("bc", witnessVersion, program);
    }

    char* address = nullptr;
    const int result = wally_scriptpubkey_to_address(
        output.script,
        output.script_len,
        WALLY_NETWORK_BITCOIN_MAINNET,
        &address);
    if (result != WALLY_OK || !address) {
        return "";
    }
    std::string value(address);
    wally_free_string(address);
    return value;
}

bool addAmount(uint64_t amount, uint64_t& total) {
    if (amount > std::numeric_limits<uint64_t>::max() - total) {
        return false;
    }
    total += amount;
    return true;
}

std::vector<uint8_t> outputScript(const wally_tx_output& output) {
    if (!output.script || output.script_len == 0) {
        return {};
    }
    return std::vector<uint8_t>(output.script, output.script + output.script_len);
}

bool transactionIdMatches(const wally_tx& transaction,
                          const unsigned char expected[WALLY_TXHASH_LEN]) {
    size_t length = 0;
    if (wally_tx_get_length(&transaction, 0, &length) != WALLY_OK || length == 0) {
        return false;
    }

    std::vector<uint8_t> serialized(length);
    size_t written = 0;
    if (wally_tx_to_bytes(&transaction, 0, serialized.data(), serialized.size(), &written) != WALLY_OK ||
        written != serialized.size()) {
        return false;
    }

    unsigned char firstHash[32];
    unsigned char transactionId[32];
    mbedtls_sha256(serialized.data(), serialized.size(), firstHash, 0);
    mbedtls_sha256(firstHash, sizeof(firstHash), transactionId, 0);
    return std::memcmp(transactionId, expected, WALLY_TXHASH_LEN) == 0;
}

bool serializeUnsignedTransaction(const wally_tx& transaction,
                                  std::vector<uint8_t>& serialized) {
    size_t length = 0;
    if (wally_tx_get_length(&transaction, 0, &length) != WALLY_OK || length == 0) {
        return false;
    }
    serialized.assign(length, 0);
    size_t written = 0;
    return wally_tx_to_bytes(
               &transaction,
               0,
               serialized.data(),
               serialized.size(),
               &written) == WALLY_OK && written == serialized.size();
}

bool samePartialSignature(const wally_partial_sigs_item& left,
                          const wally_partial_sigs_item& right) {
    return std::memcmp(left.pubkey, right.pubkey, sizeof(left.pubkey)) == 0 &&
           left.sig_len == right.sig_len &&
           left.sig && right.sig &&
           std::memcmp(left.sig, right.sig, left.sig_len) == 0;
}

bool originalSignatureIsPreserved(const wally_partial_sigs_item& original,
                                  const wally_partial_sigs_map& signedSignatures) {
    for (size_t index = 0; index < signedSignatures.num_items; ++index) {
        if (samePartialSignature(original, signedSignatures.items[index])) {
            return true;
        }
    }
    return false;
}

bool newSignatureMatchesKeypath(const wally_partial_sigs_item& signature,
                                const wally_psbt_input& originalInput) {
    if (!signature.sig || signature.sig_len < 2 ||
        signature.sig[signature.sig_len - 1] != WALLY_SIGHASH_ALL ||
        !originalInput.keypaths) {
        return false;
    }
    for (size_t index = 0; index < originalInput.keypaths->num_items; ++index) {
        if (std::memcmp(
                signature.pubkey,
                originalInput.keypaths->items[index].pubkey,
                sizeof(signature.pubkey)) == 0) {
            return true;
        }
    }
    return false;
}

} // namespace

CryptoService::CryptoService() {}

std::vector<uint8_t> CryptoService::generateRandomMbetls(size_t size) {
    // Init context
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    // Seed the DRBG
    auto pers = getRandomString(32); // length of the string
    HardwareEntropyScope entropyScope;
    const int seedResult = mbedtls_ctr_drbg_seed(
        &ctr_drbg, mbedtls_entropy_func, &entropy,
        reinterpret_cast<const unsigned char*>(pers.data()), pers.length());

    // Get random
    std::vector<uint8_t> randomData(size);
    const int randomResult = seedResult == 0
        ? mbedtls_ctr_drbg_random(&ctr_drbg, randomData.data(), size)
        : seedResult;
    // Release context
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    secureClear(pers);

    if (randomResult != 0) {
        secureClear(randomData);
        throw std::runtime_error("Failed to generate random data");
    }

    return randomData;
}

std::vector<uint8_t> CryptoService::generateRandomEsp32(size_t size) {
    // Get entropy from esp32 HRNG
    std::vector<uint8_t> randomData(size);
    HardwareEntropyScope entropyScope;
    esp_fill_random(randomData.data(), randomData.size());
    
    return randomData;
}

std::vector<uint8_t> CryptoService::generateRandomBuiltin(size_t size) {
    // Builtin esp_random
    std::vector<uint8_t> randomData(size);
    size_t i = 0;

    HardwareEntropyScope entropyScope;
    while (i < size) {
        // 32 bits integer
        uint32_t randVal = esp_random();

        // Split randVal into 4 parts
        size_t bytesToCopy = std::min(size - i, static_cast<size_t>(4));
        memcpy(randomData.data() + i, &randVal, bytesToCopy);

        i += bytesToCopy;
    }
    return randomData;
}

std::string CryptoService::getRandomString(size_t length) {
    auto randomData = generateRandomEsp32(length);
    std::string randomString(randomData.begin(), randomData.end());
    secureClear(randomData);

    return randomString;
}

std::vector<uint8_t> CryptoService::generatePrivateKey(size_t keySize) {
    if (keySize != 32) {
        throw std::invalid_argument("BIP39 24-word entropy must be 32 bytes");
    }
    std::vector<uint8_t> entropyEsp32;
    std::vector<uint8_t> entropyMbedtls;
    std::vector<uint8_t> entropyBuiltin;
    std::vector<uint8_t> entropyUser;
    std::vector<uint8_t> hashedEntropyUser;
    std::vector<uint8_t> mixedKey;
    try {
        entropyEsp32 = generateRandomEsp32(keySize);
        entropyMbedtls = generateRandomMbetls(keySize);
        entropyBuiltin = generateRandomBuiltin(keySize);
        entropyUser = entropyContext.getAccumulatedEntropy();

        if (entropyEsp32.size() != keySize || entropyMbedtls.size() != keySize ||
            entropyBuiltin.size() != keySize) {
            throw std::runtime_error("Failed to generate sufficient entropy");
        }

        hashedEntropyUser = hashSha256(entropyUser, keySize);
        mixedKey = mixEntropy(
            entropyMbedtls, entropyEsp32, entropyBuiltin, hashedEntropyUser);
        auto privateKey = hashSha256(mixedKey, keySize);

        secureClear(entropyEsp32);
        secureClear(entropyMbedtls);
        secureClear(entropyBuiltin);
        secureClear(entropyUser);
        secureClear(hashedEntropyUser);
        secureClear(mixedKey);
        return privateKey;
    } catch (...) {
        secureClear(entropyEsp32);
        secureClear(entropyMbedtls);
        secureClear(entropyBuiltin);
        secureClear(entropyUser);
        secureClear(hashedEntropyUser);
        secureClear(mixedKey);
        throw;
    }
}

std::vector<std::string> CryptoService::privateKeyToMnemonic(const std::vector<uint8_t>& privateKey) {
    // Convert a private key into a vector of words
    auto entropy = std::vector<uint8_t>(privateKey.begin(), privateKey.end());
    auto mnemonic = BIP39::create_mnemonic(entropy, BIP39::language::en);
    auto validation = BIP39::valid_mnemonic(mnemonic, BIP39::language::en);
    volatile uint8_t* entropyData = entropy.empty() ? nullptr : entropy.data();
    for (size_t index = 0; index < entropy.size(); ++index) {
        entropyData[index] = 0;
    }
    entropy.clear();
    if (!validation) {return {};}

    return {mnemonic.begin(), mnemonic.end()};;
}

std::string CryptoService::mnemonicVectorToString(const std::vector<std::string>& mnemonic) {
    std::ostringstream oss;
    for (auto it = mnemonic.begin(); it != mnemonic.end(); ++it) {
        if (it != mnemonic.begin()) {
            oss << " ";
        }
        oss << *it;
    }

    return oss.str();
}

BIP39::word_list CryptoService::mnemonicStringToWordList(const std::string& mnemonicStr) {
    BIP39::word_list wordList;

    if (mnemonicStr.empty()) {
        return wordList;
    }

    // Split the string by whitespace into tokens, then add each token to the word_list
    std::istringstream iss(mnemonicStr);
    for (std::string token; iss >> token; ) {
        wordList.add(token);
    }

    return wordList;
}

std::vector<uint8_t> CryptoService::mnemonicToPrivateKey(const std::string& mnemonic) {
    // Buffer big enough
    uint8_t buffer[64] = {0};

    // The function returns how many bytes were actually written.
    size_t written = mnemonicToEntropy(
        mnemonic.c_str(),         // The mnemonic words
        mnemonic.size(),          // Length of that string
        buffer,                   // Output buffer
        sizeof(buffer)            // Buffer size
    );

    std::vector<uint8_t> entropy(buffer, buffer + written);
    volatile uint8_t* bufferData = buffer;
    for (size_t index = 0; index < sizeof(buffer); ++index) {
        bufferData[index] = 0;
    }
    return entropy;
}

bool CryptoService::verifyMnemonic(BIP39::word_list mnemonic) {
    return BIP39::valid_mnemonic(mnemonic, BIP39::language::en);
}

std::vector<uint8_t> CryptoService::hashSha256(const std::vector<uint8_t>& entropy, size_t keySize) {
    if (keySize != 32) {
        throw std::invalid_argument("SHA-256 output size must be 32 bytes");
    }
    uint8_t hash[32];
    mbedtls_sha256(entropy.data(), entropy.size(), hash, 0); // 0 = SHA-256 (not SHA-224)

    std::vector<uint8_t> result(hash, hash + keySize);
    volatile uint8_t* hashData = hash;
    for (size_t index = 0; index < sizeof(hash); ++index) {
        hashData[index] = 0;
    }
    return result;
}

HDPublicKey CryptoService::deriveXPub(const std::string& mnemonic,
                                      const std::string& passphrase) {
    // Mnemonic words with passphrase
    HDPrivateKey hd(mnemonic.c_str(), passphrase.c_str());
    
    // derive legacy
    HDPrivateKey legacyAccount = hd.derive(getLegacyDerivePath().c_str());
    legacyAccount.type = P2PKH;
    HDPublicKey xpub = legacyAccount.xpub();
    return xpub;
}

HDPublicKey CryptoService::deriveZPub(const std::string& mnemonic,
                                      const std::string& passphrase) {
    // Mnemonic 24 words with passphrase
    HDPrivateKey hd(mnemonic.c_str(), passphrase.c_str());
    
    // derive native segwit account BIP84
    HDPrivateKey account = hd.derive(getSegwitDerivePath().c_str());
    return account.xpub();
}

std::string CryptoService::getSegwitDerivePath() {
    return "m/84'/0'/0'/";
}

std::string CryptoService::getLegacyDerivePath() {
    return "m/44'/0'/0";
}

std::string CryptoService::getFingerprint(const std::string& mnemonic,
                                          const std::string& passphrase) {
    // Mnemonic with passphrase
    HDPrivateKey hd(mnemonic.c_str(), passphrase.c_str());

    // Get fingerprint
    return hd.fingerprint().c_str();
}

std::string CryptoService::generateBitcoinSegwitAddress(HDPublicKey xpub) {
    // Set segwit addresses by default
    xpub.type = P2WPKH;
    return xpub.derive("m/0/0").address().c_str();
}

std::string CryptoService::generateBitcoinLegacyAddress(HDPublicKey xpub) {
    // Set segwit addresses by default
    xpub.type = P2PKH;
    return xpub.derive("m/0/0").address().c_str();
}

double CryptoService::calculateShanonEntropy(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        throw std::invalid_argument("Data vector is empty.");
    }

    std::map<int, int> frequency; // Frequency map to count occurrences of each byte
    for (uint8_t num : data) {
        frequency[num]++;
    }

    double entropy = 0.0;
    for (const auto& pair : frequency) {
        double p = static_cast<double>(pair.second) / data.size(); // Calculate probability
        entropy -= p * std::log2(p); // Shannon entropy formula
    }

    return entropy;
}

double CryptoService::calculateMaurerRandomness(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        throw std::invalid_argument("Data vector is empty.");
    }

    CryptoPP::MaurerRandomnessTest test;
    test.Put2(data.data(), data.size(), 0, true);

    if (test.BytesNeeded() == 0) {
        return test.GetTestValue();
    }

    throw std::runtime_error("Insufficient data for Maurer randomness test.");
}

std::vector<uint8_t> CryptoService::mixEntropy(const std::vector<uint8_t>& data1, 
                                               const std::vector<uint8_t>& data2, 
                                               const std::vector<uint8_t>& data3,
                                               const std::vector<uint8_t>& data4) {
    // Mix entropy with XOR
    std::vector<uint8_t> mixedKey(data1.size());
    for (size_t i = 0; i < data1.size(); ++i) {
        mixedKey[i] = data1[i] ^ data2[i] ^ 
                      data3[i] ^ data4[i];
    }
    return mixedKey;
}

double CryptoService::calculateMinEntropy(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        throw std::invalid_argument("Data vector is empty.");
    }

    std::map<int, int> frequency; // Frequency map
    for (uint8_t num : data) {
        frequency[num]++;
    }

    // Find the maximum probability
    size_t maxFrequency = 0;
    for (const auto& pair : frequency) {
        maxFrequency = std::max(maxFrequency, static_cast<size_t>(pair.second));
    }

    double maxProbability = static_cast<double>(maxFrequency) / data.size();
    double minEntropy = -std::log2(maxProbability); // Calculate min-entropy

    return minEntropy;
}

std::string CryptoService::encodeBase58(const uint8_t* input, size_t len) {
    if (!input || len == 0) {
        return "";
    }

    const char* base58Chars = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    size_t zeroCount = 0;

    // Count leading zero bytes
    while (zeroCount < len && input[zeroCount] == 0) {
        ++zeroCount;
    }

    // Initialize buffer for b58 conversion
    std::vector<uint8_t> b58Buffer(len * 2);
    size_t bufferSize = 0;

    // Encode to b58
    for (size_t i = zeroCount; i < len; ++i) {
        int carry = input[i];
        for (size_t j = 0; j < bufferSize; ++j) {
            carry += b58Buffer[j] * 256;
            b58Buffer[j] = carry % 58;
            carry /= 58;
        }
        while (carry > 0) {
            b58Buffer[bufferSize++] = carry % 58;
            carry /= 58;
        }
    }

    // String result
    std::string result(zeroCount, '1');
    for (auto it = b58Buffer.rbegin(); it != b58Buffer.rend(); ++it) {
        if (*it != 0 || result.size() > zeroCount) { // Skip leading zeroes in the buffer
            result += base58Chars[*it];
        }
    }

    return result;
}

std::vector<uint8_t> CryptoService::deriveKeyFromPassphrase(const std::string& passphrase, const std::string& salt, size_t keySize) {
    const std::vector<uint8_t> saltBytes(salt.begin(), salt.end());
    return deriveKeyFromPassphrase(passphrase, saltBytes, 10000, keySize);
}

std::vector<uint8_t> CryptoService::deriveKeyFromPassphrase(const std::string& passphrase,
                                                            const std::vector<uint8_t>& salt,
                                                            uint32_t iterations,
                                                            size_t keySize) {
    std::vector<uint8_t> key(keySize);

    if (passphrase.empty() || salt.empty() || iterations == 0 || keySize == 0) {
        return {};
    }

    // initialize context
    mbedtls_md_context_t mdContext;
    mbedtls_md_init(&mdContext);
    const mbedtls_md_info_t* mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);


    // Configure context
    if (mbedtls_md_setup(&mdContext, mdInfo, 1) != 0) {
        mbedtls_md_free(&mdContext);
        throw std::runtime_error("Failed to setup MD context");
    }

    // Derive key PBKDF2
    int ret = mbedtls_pkcs5_pbkdf2_hmac(
        &mdContext,                               // Contexte de hachage
        reinterpret_cast<const unsigned char*>(passphrase.data()), passphrase.size(), // Passphrase
        salt.data(), salt.size(),                 // Salt
        iterations,                               // Nombre iterations
        keySize,                                  // Taille de la clé
        key.data()                                // Résultat
    );

    mbedtls_md_free(&mdContext);

    if (ret != 0) {
        secureClear(key);
        throw std::runtime_error("Failed to derive key using PBKDF2");
    }

    return key;
}

bool CryptoService::encryptAesGcm(const std::vector<uint8_t>& plaintext,
                                  const std::vector<uint8_t>& key,
                                  const std::vector<uint8_t>& nonce,
                                  const std::vector<uint8_t>& aad,
                                  std::vector<uint8_t>& ciphertext,
                                  std::vector<uint8_t>& tag) {
    ciphertext.clear();
    tag.clear();
    if (plaintext.empty() || key.size() != 32 || nonce.size() != 12) {
        return false;
    }

    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    if (mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, key.data(), 256) != 0) {
        mbedtls_gcm_free(&context);
        return false;
    }

    ciphertext.resize(plaintext.size());
    tag.resize(16);
    const int result = mbedtls_gcm_crypt_and_tag(
        &context,
        MBEDTLS_GCM_ENCRYPT,
        plaintext.size(),
        nonce.data(), nonce.size(),
        aad.empty() ? nullptr : aad.data(), aad.size(),
        plaintext.data(), ciphertext.data(),
        tag.size(), tag.data());
    mbedtls_gcm_free(&context);

    if (result != 0) {
        ciphertext.clear();
        tag.clear();
        return false;
    }
    return true;
}

bool CryptoService::decryptAesGcm(const std::vector<uint8_t>& ciphertext,
                                  const std::vector<uint8_t>& key,
                                  const std::vector<uint8_t>& nonce,
                                  const std::vector<uint8_t>& aad,
                                  const std::vector<uint8_t>& tag,
                                  std::vector<uint8_t>& plaintext) {
    plaintext.clear();
    if (ciphertext.empty() || key.size() != 32 || nonce.size() != 12 || tag.size() != 16) {
        return false;
    }

    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    if (mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, key.data(), 256) != 0) {
        mbedtls_gcm_free(&context);
        return false;
    }

    plaintext.resize(ciphertext.size());
    const int result = mbedtls_gcm_auth_decrypt(
        &context,
        ciphertext.size(),
        nonce.data(), nonce.size(),
        aad.empty() ? nullptr : aad.data(), aad.size(),
        tag.data(), tag.size(),
        ciphertext.data(), plaintext.data());
    mbedtls_gcm_free(&context);

    if (result != 0) {
        std::fill(plaintext.begin(), plaintext.end(), 0);
        plaintext.clear();
        return false;
    }
    return true;
}

std::vector<uint8_t> CryptoService::encryptAES(const std::vector<uint8_t>& data, const std::vector<uint8_t>& key) {
    if (data.size() % 16 != 0) {
        throw std::invalid_argument("Data size must be a multiple of 16.");
    }

    if (key.size() != 16) {
        throw std::invalid_argument("Key size must be 16 bytes for AES-128.");
    }

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // Set the encryption key
    mbedtls_aes_setkey_enc(&aes, key.data(), 128);

    std::vector<uint8_t> encrypted(data.size());
    for (size_t i = 0; i < data.size(); i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, data.data() + i, encrypted.data() + i);
    }

    mbedtls_aes_free(&aes);

    return encrypted;
}

std::vector<uint8_t> CryptoService::decryptAES(const std::vector<uint8_t>& encrypted, const std::vector<uint8_t>& key) {
    if (encrypted.size() % 16 != 0) {
        throw std::invalid_argument("Encrypted data size must be a multiple of 16.");
    }

    if (key.size() != 16) {
        throw std::invalid_argument("Key size must be 16 bytes for AES-128.");
    }

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // Set the decryption key
    mbedtls_aes_setkey_dec(&aes, key.data(), 128);

    std::vector<uint8_t> decrypted(encrypted.size());
    for (size_t i = 0; i < encrypted.size(); i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, encrypted.data() + i, decrypted.data() + i);
    }

    mbedtls_aes_free(&aes);

    return decrypted;
}

std::vector<uint8_t> CryptoService::encryptPrivateKeyWithPassphrase(const std::vector<uint8_t>& privateKey, const std::string& passphrase, const std::string& salt) {
    if (privateKey.size() != 16 && privateKey.size() != 32) {
        throw std::invalid_argument("Private key size must be 16 or 32 bytes.");
    }

    // Derive key with passphrase and salt
    auto derivedKey = deriveKeyFromPassphrase(passphrase, salt, 16);

    try {
        auto encryptedPrivateKey = encryptAES(privateKey, derivedKey);
        secureClear(derivedKey);
        return encryptedPrivateKey;
    } catch (...) {
        secureClear(derivedKey);
        throw;
    }
}

std::vector<uint8_t> CryptoService::decryptPrivateKeyWithPassphrase(const std::vector<uint8_t>& encryptedPrivateKey, const std::string& passphrase, const std::string& salt) {
    if (encryptedPrivateKey.size() % 16 != 0) {
        throw std::invalid_argument("Encrypted private key size must be a multiple of 16.");
    }

    // Derive key with passphrase and salt
    auto derivedKey = deriveKeyFromPassphrase(passphrase, salt, 16);

    try {
        auto decryptedPrivateKey = decryptAES(encryptedPrivateKey, derivedKey);
        secureClear(derivedKey);
        return decryptedPrivateKey;
    } catch (...) {
        secureClear(derivedKey);
        throw;
    }
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> CryptoService::splitVector(const std::vector<uint8_t>& input) {
    if (input.size() == 16) {
        return {input, {}}; // we dont need split for 16 bytes
    }

    if (input.size() != 32) {
        throw std::invalid_argument("Input vector must be 32 bytes.");
    }

    std::vector<uint8_t> part1(input.begin(), input.begin() + 16);
    std::vector<uint8_t> part2;
    part2.insert(part2.end(), input.begin() + 16, input.end());

    return {part1, part2};
}

std::vector<uint8_t> CryptoService::generateChecksum(const std::vector<uint8_t>& data, const std::string& salt) {
    // Combine data and salt
    std::vector<uint8_t> combined(data.begin(), data.end());
    combined.insert(combined.end(), salt.begin(), salt.end());

    // Hash the combined data
    uint8_t hash[32]; // SHA256 produces 32 bytes
    mbedtls_sha256(combined.data(), combined.size(), hash, 0); // 0 for SHA256, not SHA224

    std::vector<uint8_t> checksum(hash, hash + 16);
    secureClear(combined);
    volatile uint8_t* hashData = hash;
    for (size_t index = 0; index < sizeof(hash); ++index) {
        hashData[index] = 0;
    }
    return checksum;
}

std::string CryptoService::signBitcoinTransactions(const std::string& psbtBase64, const std::string& mnemonic, const std::string& passphrase) {
    // Derive key
    HDPrivateKey rootKey(mnemonic.c_str(), passphrase.c_str());

    // Charger la PSBT
    PSBT psbt;
    size_t bytesParsed = psbt.parseBase64(psbtBase64.c_str());

    std::vector<uint8_t> signaturesBefore(psbt.tx.inputsNumber);
    for (size_t index = 0; index < psbt.tx.inputsNumber; ++index) {
        signaturesBefore[index] = psbt.txInsMeta[index].signaturesLen;
    }

    // Signer les transactions
    uint8_t signedInputs = psbt.sign(rootKey);
    if (signedInputs != psbt.tx.inputsNumber) {
        return ""; // can't sign
    }
    for (size_t index = 0; index < psbt.tx.inputsNumber; ++index) {
        if (psbt.txInsMeta[index].signaturesLen <= signaturesBefore[index]) {
            return "";
        }
    }

    // Export the signed PSBT as Base64
    std::string signedPsbtBase64 = psbt.toBase64().c_str();
    return signedPsbtBase64;
}

bool CryptoService::inspectBitcoinTransaction(const std::vector<uint8_t>& psbtBinary,
                                              const std::string& mnemonic,
                                              const std::string& passphrase,
                                              TransactionReview& review) {
    review.outputs.clear();
    review.feeSatoshis = 0;
    if (psbtBinary.empty()) {
        return false;
    }

    wally_psbt* psbt = nullptr;
    if (wally_psbt_from_bytes(psbtBinary.data(), psbtBinary.size(), &psbt) != WALLY_OK ||
        !psbt || !psbt->tx || psbt->num_inputs != psbt->tx->num_inputs ||
        psbt->num_outputs != psbt->tx->num_outputs) {
        if (psbt) {
            wally_psbt_free(psbt);
        }
        return false;
    }

    const auto base64 = convertPSBTBinaryToBase64(psbtBinary);
    PSBT ownershipPsbt;
    if (base64.empty() || ownershipPsbt.parseBase64(base64.c_str()) == 0 || !ownershipPsbt ||
        ownershipPsbt.tx.inputsNumber != psbt->num_inputs) {
        wally_psbt_free(psbt);
        return false;
    }
    HDPrivateKey rootKey(mnemonic.c_str(), passphrase.c_str());
    uint8_t rootFingerprint[4];
    rootKey.fingerprint(rootFingerprint);

    uint64_t inputSatoshis = 0;
    for (size_t index = 0; index < psbt->num_inputs; ++index) {
        const auto& input = psbt->inputs[index];
        if (!TransactionReviewService::isSupportedSighash(input.sighash_type)) {
            wally_psbt_free(psbt);
            return false;
        }
        if (input.redeem_script_len != 0 || input.witness_script_len != 0) {
            wally_psbt_free(psbt);
            return false;
        }

        bool ownedByWallet = false;
        const auto& ownershipInput = ownershipPsbt.txInsMeta[index];
        if (ownershipInput.derivationsLen != 1 || !input.keypaths ||
            input.keypaths->num_items != 1) {
            wally_psbt_free(psbt);
            return false;
        }
        for (size_t derivationIndex = 0;
             derivationIndex < ownershipInput.derivationsLen;
             ++derivationIndex) {
            const auto& derivation = ownershipInput.derivations[derivationIndex];
            if (!derivation.derivation || derivation.derivationLen != 5) {
                continue;
            }
            const std::vector<uint32_t> path(
                derivation.derivation,
                derivation.derivation + derivation.derivationLen);
            if (std::memcmp(rootFingerprint, derivation.fingerprint, sizeof(rootFingerprint)) == 0 &&
                TransactionReviewService::isSupportedBip84Path(path)) {
                const auto privateKey = rootKey.derive(
                    derivation.derivation,
                    derivation.derivationLen);
                if (derivation.pubkey == privateKey.publicKey() &&
                    privateKey.publicKey().script(P2WPKH) == ownershipInput.txOut.scriptPubkey) {
                    ownedByWallet = true;
                    break;
                }
            }
        }
        if (!ownedByWallet) {
            wally_psbt_free(psbt);
            return false;
        }

        TransactionInputEvidence evidence;
        if (input.witness_utxo) {
            evidence.hasWitnessUtxo = true;
            evidence.witnessSatoshis = input.witness_utxo->satoshi;
            evidence.witnessScript = outputScript(*input.witness_utxo);
        }
        if (input.non_witness_utxo) {
            evidence.hasNonWitnessUtxo = true;
            evidence.nonWitnessTxidMatches = transactionIdMatches(
                *input.non_witness_utxo,
                psbt->tx->inputs[index].txhash);
            const auto previousOutput = psbt->tx->inputs[index].index;
            if (previousOutput >= input.non_witness_utxo->num_outputs) {
                wally_psbt_free(psbt);
                return false;
            }
            const auto& previous = input.non_witness_utxo->outputs[previousOutput];
            evidence.nonWitnessSatoshis = previous.satoshi;
            evidence.nonWitnessScript = outputScript(previous);
        }

        uint64_t amount = 0;
        if (!TransactionReviewService::selectVerifiedInputAmount(evidence, amount)) {
            wally_psbt_free(psbt);
            return false;
        }
        if (!addAmount(amount, inputSatoshis)) {
            wally_psbt_free(psbt);
            return false;
        }
    }

    std::vector<TransactionOutputReview> outputs;
    outputs.reserve(psbt->num_outputs);
    for (size_t index = 0; index < psbt->num_outputs; ++index) {
        const auto address = outputAddress(psbt->tx->outputs[index]);
        if (address.empty()) {
            wally_psbt_free(psbt);
            return false;
        }
        outputs.push_back({
            psbt->tx->outputs[index].satoshi,
            address,
        });
    }

    wally_psbt_free(psbt);
    return TransactionReviewService::build(inputSatoshis, outputs, review);
}

bool CryptoService::mergeSignedBitcoinTransaction(
    const std::vector<uint8_t>& originalPsbt,
    const std::vector<uint8_t>& signedPsbt,
    std::vector<uint8_t>& verifiedPsbt) {
    verifiedPsbt.clear();
    if (originalPsbt.empty() || signedPsbt.empty()) {
        return false;
    }

    wally_psbt* original = nullptr;
    wally_psbt* signedTransaction = nullptr;
    const bool parsed =
        wally_psbt_from_bytes(originalPsbt.data(), originalPsbt.size(), &original) == WALLY_OK &&
        original && original->tx && original->num_inputs == original->tx->num_inputs &&
        wally_psbt_from_bytes(signedPsbt.data(), signedPsbt.size(), &signedTransaction) == WALLY_OK &&
        signedTransaction && signedTransaction->tx &&
        signedTransaction->num_inputs == signedTransaction->tx->num_inputs &&
        original->num_inputs == signedTransaction->num_inputs;
    if (!parsed) {
        if (original) {
            wally_psbt_free(original);
        }
        if (signedTransaction) {
            wally_psbt_free(signedTransaction);
        }
        return false;
    }

    std::vector<uint8_t> originalTransaction;
    std::vector<uint8_t> signedUnsignedTransaction;
    bool valid = serializeUnsignedTransaction(*original->tx, originalTransaction) &&
                 serializeUnsignedTransaction(*signedTransaction->tx, signedUnsignedTransaction) &&
                 originalTransaction == signedUnsignedTransaction;
    for (size_t index = 0; valid && index < original->num_inputs; ++index) {
        const auto* originalSignatures = original->inputs[index].partial_sigs;
        const auto* signedSignatures = signedTransaction->inputs[index].partial_sigs;
        const size_t originalCount = originalSignatures ? originalSignatures->num_items : 0;
        if (!signedSignatures || signedSignatures->num_items != originalCount + 1) {
            valid = false;
            break;
        }
        for (size_t originalIndex = 0;
             valid && originalIndex < originalCount;
             ++originalIndex) {
            valid = originalSignatureIsPreserved(
                originalSignatures->items[originalIndex],
                *signedSignatures);
        }

        bool foundNewSignature = false;
        for (size_t signedIndex = 0;
             valid && signedIndex < signedSignatures->num_items;
             ++signedIndex) {
            bool existedBefore = false;
            for (size_t originalIndex = 0; originalIndex < originalCount; ++originalIndex) {
                if (samePartialSignature(
                        signedSignatures->items[signedIndex],
                        originalSignatures->items[originalIndex])) {
                    existedBefore = true;
                    break;
                }
            }
            if (!existedBefore) {
                valid = !foundNewSignature && newSignatureMatchesKeypath(
                    signedSignatures->items[signedIndex],
                    original->inputs[index]);
                foundNewSignature = valid;
            }
        }
        valid = valid && foundNewSignature;
    }

    for (size_t index = 0; valid && index < original->num_inputs; ++index) {
        valid = signedTransaction->inputs[index].partial_sigs &&
                wally_psbt_input_set_partial_sigs(
                    &original->inputs[index],
                    signedTransaction->inputs[index].partial_sigs) == WALLY_OK;
    }
    size_t serializedLength = 0;
    if (valid) {
        valid = wally_psbt_get_length(original, &serializedLength) == WALLY_OK &&
                serializedLength > 0;
    }
    if (valid) {
        verifiedPsbt.assign(serializedLength, 0);
        size_t written = 0;
        valid = wally_psbt_to_bytes(
                    original,
                    verifiedPsbt.data(),
                    verifiedPsbt.size(),
                    &written) == WALLY_OK && written == verifiedPsbt.size();
    }

    wally_psbt_free(original);
    wally_psbt_free(signedTransaction);
    if (!valid) {
        verifiedPsbt.clear();
    }
    return valid;
}

std::string CryptoService::convertPSBTBinaryToBase64(const std::vector<uint8_t>& psbtBinary) {
    // Parse the binary PSBT
    wally_psbt* psbt = nullptr;
    int res = wally_psbt_from_bytes(psbtBinary.data(), psbtBinary.size(), &psbt);
    if (res != WALLY_OK || psbt == nullptr) {
        return "";
    }

    // Convert the structure to Base64
    char* psbtBase64 = nullptr;
    res = wally_psbt_to_base64(psbt, &psbtBase64);
    if (res != WALLY_OK || psbtBase64 == nullptr) {
        wally_psbt_free(psbt);
        return "";
    }

    // Store the Base64 string in a `std::string`
    std::string psbtBase64Str(psbtBase64);

    // Clean up memory
    wally_free_string(psbtBase64); 
    wally_psbt_free(psbt);

    return psbtBase64Str;
}

std::vector<uint8_t> CryptoService::convertPSBTBase64ToBinary(const std::string& psbtBase64) {
    // Parse PSBT Base64
    wally_psbt* psbt = nullptr;
    int res = wally_psbt_from_base64(psbtBase64.c_str(), &psbt);
    if (res != WALLY_OK || psbt == nullptr) {
        return {};
    }

    // Get length
    size_t psbtBinaryLen = 0;
    res = wally_psbt_get_length(psbt, &psbtBinaryLen);
    if (res != WALLY_OK || psbtBinaryLen == 0) {
        wally_psbt_free(psbt);
        return {};
    }

    // Convertir en binaire
    std::vector<uint8_t> psbtBinary(psbtBinaryLen);
    size_t bytesWritten = 0;
    res = wally_psbt_to_bytes(psbt, psbtBinary.data(), psbtBinaryLen, &bytesWritten);
    if (res != WALLY_OK || bytesWritten != psbtBinaryLen) {
        wally_psbt_free(psbt); // Nettoyer la mémoire
        return {};
    }

    wally_psbt_free(psbt);

    return psbtBinary;
}

std::vector<uint8_t> CryptoService::OLDderivePublicKey(const std::vector<uint8_t>& privateKey) {
    // Create context
    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

    // Verify Private key
    if (!secp256k1_ec_seckey_verify(ctx, privateKey.data())) {
        secp256k1_context_destroy(ctx);
        throw std::runtime_error("Invalid private key");
    }

    // Get PubKey
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_create(ctx, &pubkey, privateKey.data())) {
        secp256k1_context_destroy(ctx);
        throw std::runtime_error("Failed to create public key");
    }
    
    // Serialize uncompressed PubKey
    unsigned char serializedPubkey[65];
    size_t serializedPubkeyLen = sizeof(serializedPubkey);
    secp256k1_ec_pubkey_serialize(ctx, serializedPubkey, &serializedPubkeyLen, &pubkey, SECP256K1_EC_UNCOMPRESSED);
    
    // Delete context
    secp256k1_context_destroy(ctx);

    return std::vector<uint8_t>(serializedPubkey, serializedPubkey + serializedPubkeyLen);
}

void CryptoService::OLDhashPublicKey(const std::vector<uint8_t>& publicKey, std::vector<uint8_t>& hashedKey) {
    uint8_t sha256Hash[32];
    mbedtls_sha256(publicKey.data(), publicKey.size(), sha256Hash, 0);

    CryptoPP::RIPEMD160 ripemd;
    hashedKey.resize(20);
    ripemd.CalculateDigest(hashedKey.data(), sha256Hash, sizeof(sha256Hash));
}

std::string CryptoService::OLDgenerateBitcoinSegwitAddress(const std::vector<uint8_t>& publicKey) {
    // Legacy address starting with 1.....
    std::vector<uint8_t> hashedKey(20);
    OLDhashPublicKey(publicKey, hashedKey);

    uint8_t extendedKey[25];
    uint8_t checksum[32];
    extendedKey[0] = 0x00;
    std::memcpy(extendedKey + 1, hashedKey.data(), 20);

    mbedtls_sha256(extendedKey, 21, checksum, 0);
    mbedtls_sha256(checksum, 32, checksum, 0);

    std::memcpy(extendedKey + 21, checksum, 4);
    return encodeBase58(extendedKey, 25);
}

}
