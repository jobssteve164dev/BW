#ifndef TEST_WALLET_MANAGER_H
#define TEST_WALLET_MANAGER_H

#include "Contexts/GlobalContext.h"
#include "Contexts/SelectionContext.h"
#include "Enums/WalletInformationEnum.h"

#include <cstdint>
#include <string>
#include <vector>

using namespace enums;

namespace views {
class CardputerView {};
}

namespace inputs {
class CardputerInput {};
}

namespace services {

class UsbService {
public:
    void setLayout(const uint8_t*) { ++setLayoutCalls; }
    void begin() { ++beginCalls; }
    int setLayoutCalls = 0;
    int beginCalls = 0;
};

class LedService {};

} // namespace services

namespace selections {

class KeyboardLayoutSelection {
public:
    const uint8_t* select() {
        ++selectCalls;
        return layout;
    }
    int selectCalls = 0;

private:
    uint8_t layout[1] = {0};
};

class WalletInformationSelection {
public:
    enums::WalletInformationEnum select(const std::string&) { return next; }
    enums::WalletInformationEnum next = enums::WalletInformationEnum::NONE;
};

class ValueSelection {
public:
    void select(const std::string& description,
                const std::string& value,
                services::UsbService&,
                services::LedService&,
                KeyboardLayoutSelection&) {
        ++selectCalls;
        lastDescription = description;
        lastValue = value;
    }

    int selectCalls = 0;
    std::string lastDescription;
    std::string lastValue;
};

class ConfirmationSelection {
public:
    bool select(const std::string&) { return false; }
};

class WalletSelection {
public:
    models::Wallet select(const std::vector<models::Wallet>&) { return {}; }
};

} // namespace selections

namespace managers {

class WalletService {
public:
    std::vector<models::Wallet> getAllWallets() const { return wallets; }
    std::vector<models::Wallet> wallets;
};

class Display {
public:
    void displaySubMessage(const std::string&, size_t, int) {}
};

class WalletManager {
public:
    WalletService walletService;
    selections::ConfirmationSelection confirmationSelection;
    selections::WalletSelection walletSelection;
    selections::WalletInformationSelection walletInformationSelection;
    selections::ValueSelection valueSelection;
    selections::KeyboardLayoutSelection keyboardLayoutSelection;
    services::UsbService usbService;
    services::LedService ledService;
    Display display;

    void endTransactionSigning() {}
};

} // namespace managers

#endif // TEST_WALLET_MANAGER_H
