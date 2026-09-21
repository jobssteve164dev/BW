#ifndef SETTINGS_SERVICE_H
#define SETTINGS_SERVICE_H

#include <string>

namespace services {

class SettingsService {
public:
    std::string loadWalletPath() const;
    bool saveWalletPath(const std::string& path) const;
};

} // namespace services

#endif // SETTINGS_SERVICE_H
