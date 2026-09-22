#include <cassert>
#include <iostream>

#include "Views/CardputerView.h"

M5CardputerClass M5Cardputer;
fonts::Font fonts::efontCN_16;
fonts::Font fonts::FreeSerifBold24pt7b;
fonts::Font fonts::Font0;

using views::CardputerView;

namespace {

const TextCall& drawnText(const std::string& text) {
    for (const auto& call : M5Cardputer.Display.drawnStrings) {
        if (call.text == text) return call;
    }
    assert(false && "expected drawn text was not rendered");
    return M5Cardputer.Display.drawnStrings.front();
}

void testBitcoinIconMatchesTheConfirmedSixteenPixelScale() {
    CardputerView::initialize();
    M5Cardputer.Display.clearRecordedCalls();

    CardputerView::drawBitcoinIcon(6, 6);

    assert(M5Cardputer.Display.filledCircles.size() == 1);
    const auto icon = M5Cardputer.Display.filledCircles.front();
    assert(icon.radius * 2 == 16);
    assert(icon.y == 14);
    assert(M5Cardputer.Display.drawnStrings.size() == 1);
    const auto label = M5Cardputer.Display.drawnStrings.front();
    assert(label.text == "B");
    assert(label.font == &fonts::Font0);
    assert(label.x == icon.x);
    assert(label.y >= icon.y - icon.radius && label.y <= icon.y + icon.radius);
    assert(M5Cardputer.Display.currentFont == &fonts::efontCN_16);
}

void testTopBarUsesOneVisualCenterAndKeepsStableClearances() {
    CardputerView::initialize();
    M5Cardputer.Display.clearRecordedCalls();

    CardputerView::displayTopBar("Wallet", true, true, false, 20);

    const auto back = drawnText("<");
    const auto title = drawnText("Wallet");
    assert(back.x == 12);
    assert(back.y == 14);
    assert(back.datum == middle_center);
    assert(title.y == 14);
    assert(title.x == 89);
    assert(title.datum == middle_center);

    assert(M5Cardputer.Display.outlinedCircles.size() == 1);
    const auto search = M5Cardputer.Display.outlinedCircles.front();
    assert(search.x == 164);
    assert(search.y == 13);
    assert(search.radius == 7);
    assert(title.x + M5Cardputer.Display.textWidth(title.text.c_str()) / 2 <= search.x - search.radius - 6);

    assert(M5Cardputer.Display.drawnRects.size() == 1);
    const auto battery = M5Cardputer.Display.drawnRects.front();
    assert(battery.x == 183);
    assert(battery.y == 8);
    assert(battery.width == 18);
    assert(battery.height == 10);
    const auto percentage = drawnText("100%");
    assert(percentage.x == 220);
    assert(percentage.y == 15);
    assert(percentage.datum == middle_center);
}

void testQrOverlayPlacesBitcoinIconClearOfTheQrCodeAtTopLeft() {
    CardputerView::initialize();
    M5Cardputer.Display.clearRecordedCalls();

    CardputerView::displayQrCode("bc1qexample");
    CardputerView::displayTopIcon();

    assert(M5Cardputer.Display.qrCodes.size() == 1);
    assert(M5Cardputer.Display.filledCircles.size() == 1);
    const auto qr = M5Cardputer.Display.qrCodes.front();
    const auto icon = M5Cardputer.Display.filledCircles.front();
    const int qrLeft = qr.x < 0 ? (M5Cardputer.Display.width() - qr.size) / 2 : qr.x;
    const int iconLeft = icon.x - icon.radius;
    const int iconTop = icon.y - icon.radius;
    const int iconRight = icon.x + icon.radius;
    const auto back = drawnText("<");
    const auto battery = M5Cardputer.Display.drawnRects.front();
    const int qrRight = qrLeft + qr.size;

    assert(iconLeft == 4);
    assert(iconTop == 6);
    assert(iconRight < qrLeft);
    assert(back.x == 33);
    assert(back.y == 14);
    assert(battery.x > qrRight);
    for (const auto& rect : M5Cardputer.Display.filledRects) {
        const bool overlapsQrHorizontally = rect.x < qrRight && rect.x + rect.width > qrLeft;
        const bool overlapsQrVertically = rect.y < qr.y + qr.size && rect.y + rect.height > qr.y;
        assert(!(overlapsQrHorizontally && overlapsQrVertically));
    }
}

} // namespace

int main() {
    testBitcoinIconMatchesTheConfirmedSixteenPixelScale();
    testTopBarUsesOneVisualCenterAndKeepsStableClearances();
    testQrOverlayPlacesBitcoinIconClearOfTheQrCodeAtTopLeft();
    std::cout << "brand icon layout tests passed\n";
    return 0;
}
