#ifndef WALLET_FILE_SCOPE_H
#define WALLET_FILE_SCOPE_H

#include <cctype>
#include <string>

namespace services {

class WalletFileScope {
public:
    static std::string directory(const std::string& fingerprint) {
        if (fingerprint.empty() || fingerprint.size() > 8) {
            return "";
        }

        std::string normalized(8 - fingerprint.size(), '0');
        normalized.reserve(8);
        for (const unsigned char value : fingerprint) {
            if (!std::isxdigit(value)) {
                return "";
            }
            normalized.push_back(static_cast<char>(std::toupper(value)));
        }
        return "/BW/wallets/" + normalized;
    }

    static bool contains(const std::string& directory,
                         const std::string& path) {
        if (directory.empty() || path.size() <= directory.size() ||
            path.compare(0, directory.size(), directory) != 0) {
            return false;
        }
        return path[directory.size()] == '/';
    }
};

} // namespace services

#endif // WALLET_FILE_SCOPE_H
