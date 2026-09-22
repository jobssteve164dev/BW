#ifndef TEST_MODE_GLOBAL_CONTEXT_H
#define TEST_MODE_GLOBAL_CONTEXT_H

#include <string>

namespace contexts {

class GlobalContext {
public:
    static GlobalContext& getInstance() {
        static GlobalContext instance;
        return instance;
    }

    std::string getAppName() const { return "BW"; }
};

} // namespace contexts

#endif // TEST_MODE_GLOBAL_CONTEXT_H
