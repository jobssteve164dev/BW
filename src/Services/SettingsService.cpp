#include "SettingsService.h"

#include <Preferences.h>

namespace services {
namespace {

constexpr const char* SETTINGS_NAMESPACE = "bw-settings";
constexpr const char* WALLET_PATH_KEY = "wallet-path";

} // namespace

std::string SettingsService::loadWalletPath() const {
    Preferences preferences;
    if (!preferences.begin(SETTINGS_NAMESPACE, true)) {
        return {};
    }
    const String value = preferences.getString(WALLET_PATH_KEY, "");
    preferences.end();
    return std::string(value.c_str());
}

bool SettingsService::saveWalletPath(const std::string& path) const {
    if (path.empty()) {
        return false;
    }

    Preferences preferences;
    if (!preferences.begin(SETTINGS_NAMESPACE, false)) {
        return false;
    }
    const size_t written = preferences.putString(WALLET_PATH_KEY, path.c_str());
    preferences.end();
    return written == path.size();
}

} // namespace services
