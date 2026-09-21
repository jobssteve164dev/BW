#include "ValueSelection.h"

namespace selections {

ValueSelection::ValueSelection(CardputerView& display, CardputerInput& input)
    : display(display), input(input) {}

void ValueSelection::select(const std::string& description,
                            const std::string& value,
                            UsbService& usbService,
                            LedService& ledService,
                            KeyboardLayoutSelection& keyboardLayoutSelection) {
    char key = KEY_NONE;
    WalletValueViewport viewport(
        value,
        WalletValueLayout::CHARACTERS_PER_LINE,
        WalletValueLayout::VISIBLE_LINE_COUNT
    );
    auto& selectionContext = contexts::SelectionContext::getInstance();

    const auto redraw = [&]() {
        display.displayTopBar(description, true, false, false, 20);
        display.displayWalletValue(
            viewport.visibleLines(),
            viewport.canScrollUp(),
            viewport.canScrollDown()
        );
    };

    redraw();

    while (key != KEY_RETURN_CUSTOM) {
        key = input.handler();

        switch (key) {
            case KEY_OK: // Send USB
                if (!selectionContext.getIsLayoutSelected()) {
                    const uint8_t* selectedLayout = keyboardLayoutSelection.select();
                    usbService.setLayout(selectedLayout);
                    usbService.begin();
                    selectionContext.setIsLayoutSelected(true);
                    redraw();
                    delay(1500); // HID needs time to become available after initialization.
                }
                ledService.showLed();
                usbService.sendString(value);
                ledService.clearLed();
                break;
            case KEY_ARROW_UP:
                if (viewport.scrollUp()) redraw();
                break;
            case KEY_ARROW_DOWN:
                if (viewport.scrollDown()) redraw();
                break;
            case 'q': // QR Code
                display.setBrightness(50);
                display.displayQrCode(value);
                display.displayTopIcon();
                input.waitPress();
                display.setBrightness(120);
                redraw();
                break;
        }
    }
}

} // namespace selections
