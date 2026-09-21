#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "Services/WalletValueViewport.h"
#include "Services/WalletValueLayout.h"

using services::WalletValueViewport;
using services::WalletValueLayout;

namespace {

void testLongValueIsPreservedAcrossWrappedLines() {
    const std::string value =
        "https://www.blockonomics.co/#/search?q=zpub6rLongWalletValueForBalanceLookup";
    WalletValueViewport viewport(value, 12, 3);

    std::string reconstructed;
    for (const auto& line : viewport.allLines()) {
        reconstructed += line;
        assert(line.size() <= 12);
    }

    assert(reconstructed == value);
    assert(viewport.visibleLines().size() == 3);
    assert(!viewport.canScrollUp());
    assert(viewport.canScrollDown());
}

void testArrowNavigationMovesOneLineAndStopsAtBoundaries() {
    WalletValueViewport viewport("abcdefghijklmnopqrstuvwxyz0123456789", 6, 3);

    assert(viewport.firstVisibleLine() == 0);
    assert(!viewport.scrollUp());
    assert(viewport.scrollDown());
    assert(viewport.firstVisibleLine() == 1);
    assert(viewport.visibleLines() == std::vector<std::string>({"ghijkl", "mnopqr", "stuvwx"}));

    while (viewport.scrollDown()) {}
    assert(!viewport.canScrollDown());
    assert(viewport.firstVisibleLine() == 3);
    assert(viewport.visibleLines() == std::vector<std::string>({"stuvwx", "yz0123", "456789"}));
    assert(viewport.scrollUp());
    assert(viewport.firstVisibleLine() == 2);
}

void testShortAndEmptyValuesRemainSafe() {
    WalletValueViewport shortValue("bc1qaddress", 26, 3);
    assert(shortValue.visibleLines() == std::vector<std::string>({"bc1qaddress"}));
    assert(!shortValue.canScrollUp());
    assert(!shortValue.canScrollDown());

    WalletValueViewport emptyValue("", 26, 3);
    assert(emptyValue.visibleLines() == std::vector<std::string>({""}));
    assert(!emptyValue.scrollDown());
}

void testThreeLineLayoutFitsAboveActionsAndBesideIndicators() {
    const int lastLineCenter = WalletValueLayout::FIRST_LINE_CENTER_Y +
        (WalletValueLayout::VISIBLE_LINE_COUNT - 1) * WalletValueLayout::LINE_SPACING;
    const int lastLineBottom = lastLineCenter + WalletValueLayout::FONT_HEIGHT / 2;
    const int contentRight = WalletValueLayout::CONTENT_CENTER_X +
        WalletValueLayout::CHARACTERS_PER_LINE * WalletValueLayout::ASCII_GLYPH_WIDTH / 2;
    const int indicatorLeft = WalletValueLayout::INDICATOR_CENTER_X -
        WalletValueLayout::ASCII_GLYPH_WIDTH / 2;

    assert(lastLineBottom < WalletValueLayout::ACTION_AREA_TOP);
    assert(contentRight < indicatorLeft);
    assert(WalletValueLayout::INDICATOR_CENTER_X + WalletValueLayout::ASCII_GLYPH_WIDTH / 2 <=
           WalletValueLayout::SCREEN_WIDTH);
    assert(WalletValueLayout::ACTION_AREA_TOP + WalletValueLayout::ACTION_ROW_SPACING +
           WalletValueLayout::ACTION_HEIGHT <= WalletValueLayout::SCREEN_HEIGHT);
}

} // namespace

int main() {
    testLongValueIsPreservedAcrossWrappedLines();
    testArrowNavigationMovesOneLineAndStopsAtBoundaries();
    testShortAndEmptyValuesRemainSafe();
    testThreeLineLayoutFitsAboveActionsAndBesideIndicators();
    std::cout << "wallet value viewport tests passed\n";
    return 0;
}
