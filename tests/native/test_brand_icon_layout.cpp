#include <cassert>
#include <iostream>

#include "Views/CardputerView.h"

M5CardputerClass M5Cardputer;
fonts::Font fonts::efontCN_16;
fonts::Font fonts::FreeSerifBold24pt7b;
fonts::Font fonts::Font0;

using views::CardputerView;

namespace {

void testBitcoinIconIsAtLeastHalfSmaller() {
    CardputerView::initialize();
    M5Cardputer.Display.clearRecordedCalls();

    CardputerView::drawBitcoinIcon(20, 3);

    assert(M5Cardputer.Display.filledCircles.size() == 1);
    const auto icon = M5Cardputer.Display.filledCircles.front();
    assert(icon.radius * 2 <= 11);
    assert(M5Cardputer.Display.drawnStrings.size() == 1);
    const auto label = M5Cardputer.Display.drawnStrings.front();
    assert(label.text == "B");
    assert(label.font == &fonts::Font0);
    assert(label.x == icon.x);
    assert(label.y >= icon.y - icon.radius && label.y <= icon.y + icon.radius);
    assert(M5Cardputer.Display.currentFont == &fonts::efontCN_16);
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

    assert(iconLeft >= 0);
    assert(iconLeft <= 4);
    assert(iconTop >= 0);
    assert(iconTop <= 4);
    assert(iconRight < qrLeft);
}

} // namespace

int main() {
    testBitcoinIconIsAtLeastHalfSmaller();
    testQrOverlayPlacesBitcoinIconClearOfTheQrCodeAtTopLeft();
    std::cout << "brand icon layout tests passed\n";
    return 0;
}
