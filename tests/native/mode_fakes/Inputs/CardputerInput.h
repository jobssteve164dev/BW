#ifndef TEST_MODE_CARDPUTER_INPUT_H
#define TEST_MODE_CARDPUTER_INPUT_H

#include <cstddef>
#include <vector>

#define KEY_OK '\n'
#define KEY_NONE '\0'
#define KEY_ARROW_UP ';'
#define KEY_ARROW_DOWN '.'
#define KEY_ARROW_RIGHT '/'

namespace inputs {

class CardputerInput {
public:
    explicit CardputerInput(const std::vector<char>& keys) : keys(keys) {}

    char handler() {
        return next < keys.size() ? keys[next++] : KEY_OK;
    }

private:
    std::vector<char> keys;
    size_t next = 0;
};

} // namespace inputs

#endif // TEST_MODE_CARDPUTER_INPUT_H
