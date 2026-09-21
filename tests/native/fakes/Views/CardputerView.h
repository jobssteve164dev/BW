#ifndef TEST_CARDPUTER_VIEW_H
#define TEST_CARDPUTER_VIEW_H

#include <string>
#include <vector>

namespace views {

struct WalletValueFrame {
    std::vector<std::string> lines;
    bool canScrollUp;
    bool canScrollDown;
};

class CardputerView {
public:
    void displayTopBar(const std::string&, bool, bool, bool, size_t) {}

    void displayWalletValue(const std::vector<std::string>& lines, bool canScrollUp, bool canScrollDown) {
        frames.push_back({lines, canScrollUp, canScrollDown});
    }

    void setBrightness(unsigned short) {}
    void displayQrCode(const std::string&) {}
    void displayTopIcon() {}

    std::vector<WalletValueFrame> frames;
};

} // namespace views

#endif // TEST_CARDPUTER_VIEW_H
