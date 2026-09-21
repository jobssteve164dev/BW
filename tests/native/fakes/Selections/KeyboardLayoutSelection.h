#ifndef TEST_KEYBOARD_LAYOUT_SELECTION_H
#define TEST_KEYBOARD_LAYOUT_SELECTION_H

#include <cstdint>
#include <string>
#include <vector>

namespace selections {

class KeyboardLayoutSelection {
public:
    explicit KeyboardLayoutSelection(std::vector<std::string>& trace) : trace(trace) {}

    const uint8_t* select() {
        trace.push_back("selectLayout");
        return layout;
    }

private:
    std::vector<std::string>& trace;
    uint8_t layout[1] = {0};
};

} // namespace selections

#endif // TEST_KEYBOARD_LAYOUT_SELECTION_H
