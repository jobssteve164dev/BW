#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "Selections/ValueSelection.h"

using contexts::SelectionContext;
using inputs::CardputerInput;
using selections::KeyboardLayoutSelection;
using selections::ValueSelection;
using services::LedService;
using services::UsbService;
using views::CardputerView;

namespace {

void resetLayoutSelection() {
    SelectionContext::getInstance().setIsLayoutSelected(false);
}

void testOpeningValueDoesNotRequestLayoutOrStartUsb() {
    resetLayoutSelection();
    std::vector<std::string> trace;
    CardputerView display;
    CardputerInput input({KEY_RETURN_CUSTOM});
    UsbService usb(trace);
    LedService led;
    KeyboardLayoutSelection layout(trace);
    ValueSelection selection(display, input);

    selection.select("地址", "bc1qaddress", usb, led, layout);

    assert(trace.empty());
    assert(display.frames.size() == 1);
}

void testArrowKeysScrollAndRedrawTheActualSelection() {
    resetLayoutSelection();
    std::vector<std::string> trace;
    CardputerView display;
    CardputerInput input({KEY_ARROW_DOWN, KEY_ARROW_UP, KEY_RETURN_CUSTOM});
    UsbService usb(trace);
    LedService led;
    KeyboardLayoutSelection layout(trace);
    ValueSelection selection(display, input);
    const std::string value(100, 'x');

    selection.select("余额", value, usb, led, layout);

    assert(trace.empty());
    assert(display.frames.size() == 3);
    assert(!display.frames[0].canScrollUp);
    assert(display.frames[0].canScrollDown);
    assert(display.frames[1].canScrollUp);
    assert(display.frames[2].lines == display.frames[0].lines);
}

void testFirstOkInitializesUsbOnceBeforeSending() {
    resetLayoutSelection();
    std::vector<std::string> trace;
    CardputerView display;
    CardputerInput input({KEY_OK, KEY_OK, KEY_RETURN_CUSTOM});
    UsbService usb(trace);
    LedService led;
    KeyboardLayoutSelection layout(trace);
    ValueSelection selection(display, input);

    selection.select("地址", "bc1qaddress", usb, led, layout);

    assert(trace == std::vector<std::string>({
        "selectLayout", "setLayout", "begin", "send", "send"
    }));
    assert(SelectionContext::getInstance().getIsLayoutSelected());
}

} // namespace

int main() {
    testOpeningValueDoesNotRequestLayoutOrStartUsb();
    testArrowKeysScrollAndRedrawTheActualSelection();
    testFirstOkInitializesUsbOnceBeforeSending();
    std::cout << "value selection interaction tests passed\n";
    return 0;
}
