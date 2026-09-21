#include <M5Cardputer.h>
#include <Views/CardputerView.h>
#include <Inputs/CardputerInput.h>
#include <Dispatchers/AppDispatcher.h>
#include <Services/Esp32WirelessRadioControl.h>
#include <Services/WirelessSecurity.h>

using namespace dispatchers;
using namespace services;

CardputerView display;
CardputerInput input;
AppDispatcher* dispatcher;

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    Esp32WifiRadio wifi;
    Esp32BluetoothRadio bluetooth;
    if (!enforceWirelessIsolation(wifi, bluetooth)) {
        display.initialize();
        display.displayTopBar("安全启动失败");
        display.displaySubMessage("无线模块未关闭", 58);
        while (true) {
            delay(1000);
        }
    }

    dispatcher = new AppDispatcher(display, input);
    dispatcher->setup();
}

void loop() {
    dispatcher->run();
}
