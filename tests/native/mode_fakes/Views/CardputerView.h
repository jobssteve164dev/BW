#ifndef TEST_MODE_CARDPUTER_VIEW_H
#define TEST_MODE_CARDPUTER_VIEW_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace views {

struct TopBarCall {
    std::string title;
    bool submenu;
    bool searchBar;
    bool bitcoinIcon;
    size_t correctionOffset;
};

class CardputerView {
public:
    void displayTopBar(const std::string& title,
                       bool submenu = false,
                       bool searchBar = false,
                       bool bitcoinIcon = false,
                       size_t correctionOffset = 0) {
        topBars.push_back({title, submenu, searchBar, bitcoinIcon, correctionOffset});
    }

    void drawBitcoinIcon(int, int) { ++directBitcoinIconCalls; }

    void displaySelection(const std::vector<std::string>&,
                          uint16_t,
                          const std::vector<std::string>&,
                          bool = false,
                          bool = false) {}

    std::vector<TopBarCall> topBars;
    int directBitcoinIconCalls = 0;
};

} // namespace views

#endif // TEST_MODE_CARDPUTER_VIEW_H
