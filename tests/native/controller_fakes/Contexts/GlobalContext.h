#ifndef TEST_GLOBAL_CONTEXT_H
#define TEST_GLOBAL_CONTEXT_H

#include <string>

namespace contexts {

class GlobalContext {
public:
    static GlobalContext& getInstance() {
        static GlobalContext instance;
        return instance;
    }

    std::string getBitcoinBalanceUrl() const { return "https://balance.example/?q="; }
};

} // namespace contexts

#endif // TEST_GLOBAL_CONTEXT_H
