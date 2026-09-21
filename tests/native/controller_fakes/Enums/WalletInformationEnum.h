#ifndef TEST_WALLET_INFORMATION_ENUM_H
#define TEST_WALLET_INFORMATION_ENUM_H

namespace enums {

enum class WalletInformationEnum {
    NONE,
    BALANCE,
    ADDRESS,
    SIGNATURE,
    PUBLIC_KEY,
    FINGERPRINT,
    DERIVE_PATH
};

enum class SelectionModeEnum { LOAD_SD };
enum class FileTypeEnum { WALLET, TRANSACTION };

} // namespace enums

#endif // TEST_WALLET_INFORMATION_ENUM_H
