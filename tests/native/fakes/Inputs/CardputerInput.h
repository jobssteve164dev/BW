#ifndef TEST_CARDPUTER_INPUT_H
#define TEST_CARDPUTER_INPUT_H

#include <cstddef>
#include <vector>

#define KEY_OK '\n'
#define KEY_NONE '\0'
#define KEY_RETURN_CUSTOM '\r'
#define KEY_ARROW_UP ';'
#define KEY_ARROW_DOWN '.'

namespace inputs {

class CardputerInput {
public:
    explicit CardputerInput(const std::vector<char>& keys) : keys(keys) {}

    char handler() {
        return next < keys.size() ? keys[next++] : KEY_RETURN_CUSTOM;
    }

    void waitPress() {}

private:
    std::vector<char> keys;
    size_t next = 0;
};

} // namespace inputs

#endif // TEST_CARDPUTER_INPUT_H
