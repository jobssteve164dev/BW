#include <cassert>
#include <iostream>

#include "Models/Wallet.h"

using models::Wallet;

int main() {
    Wallet wallet("name", "zpub", "bc1", "fingerprint", "m/84'/0'/0'");
    assert(!wallet.hasLoadedSecrets());

    wallet.setPassphrase("");
    wallet.setMnemonic("abandon abandon abandon");
    assert(wallet.hasLoadedSecrets());
    assert(wallet.getPassphrase().empty());

    Wallet copy = wallet;
    copy.clearSecrets();
    assert(!copy.hasLoadedSecrets());
    assert(copy.getMnemonic().empty());
    assert(wallet.hasLoadedSecrets());

    wallet.clearSecrets();
    assert(!wallet.hasLoadedSecrets());
    assert(wallet.getMnemonic().empty());
    std::cout << "wallet secret state tests passed\n";
    return 0;
}
