#include <cassert>
#include <iostream>
#include <vector>

#include "Selections/ModeSelection.h"

using inputs::CardputerInput;
using selections::ModeSelection;
using views::CardputerView;

int main() {
    CardputerView display;
    CardputerInput input({KEY_OK});
    ModeSelection selection(display, input);

    selection.select();

    assert(display.topBars.size() == 1);
    const auto& topBar = display.topBars.front();
    assert(topBar.title == "BW");
    assert(!topBar.submenu);
    assert(!topBar.searchBar);
    assert(topBar.bitcoinIcon);
    assert(topBar.correctionOffset == 0);
    assert(display.directBitcoinIconCalls == 0);
    std::cout << "mode status bar test passed\n";
    return 0;
}
