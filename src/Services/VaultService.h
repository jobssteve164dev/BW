#ifndef VAULT_SERVICE_H
#define VAULT_SERVICE_H

#include <string>
#include <vector>

#include <Services/CryptoService.h>
#include <Services/SdService.h>
#include <Services/VaultCodec.h>

namespace services {

enum class VaultStatus {
    OK,
    NOT_FOUND,
    INVALID_FORMAT,
    AUTH_FAILED,
    IO_ERROR,
    CRYPTO_ERROR
};

class VaultService {
public:
    static constexpr const char* VAULT_PATH = "/bw-vault.dat";
    static constexpr const char* TEMPORARY_PATH = "/bw-vault.tmp";
    static constexpr const char* BACKUP_PATH = "/bw-vault.bak";
    static constexpr const char* CORRUPT_PATH = "/bw-vault.bad";
    static constexpr uint32_t KDF_ITERATIONS = 200000;

    VaultService(CryptoService& cryptoService, SdService& sdService);

    bool exists();
    VaultStatus inspect();
    VaultStatus load(const std::string& password, std::vector<VaultRecord>& records);
    VaultStatus upsert(const std::string& password, const VaultRecord& record);
    static void clearRecord(VaultRecord& record);
    static void clearRecords(std::vector<VaultRecord>& records);

private:
    VaultStatus loadFromPath(const char* path,
                             const std::string& password,
                             std::vector<VaultRecord>& records);
    static void secureClear(std::vector<uint8_t>& value);
    static void secureClear(std::string& value);

    CryptoService& cryptoService;
    SdService& sdService;
    bool loadedFromBackup = false;
};

} // namespace services

#endif // VAULT_SERVICE_H
