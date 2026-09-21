#include "UsbService.h"

namespace services {

UsbService::UsbService() 
    : keyboard(), layout(KeyboardLayout_en_US) {}

void UsbService::setLayout(const uint8_t* newLayout) {
    layout = newLayout;
}

void UsbService::begin() {
    if (!initialized) {
        USB.begin();
        keyboard.begin(layout);
        initialized = true;
    }
}

void UsbService::end() {
    keyboard.end();
}

// USB Keyboard send string
void UsbService::sendString(const std::string& text) {
    keyboard.releaseAll();

    for (const char& c : text) {
        auto sent = keyboard.write(c);
        if (sent == 0) {
            M5Cardputer.Display.fillScreen(TFT_BLACK);
            M5Cardputer.Display.setFont(&fonts::efontCN_16);
            M5Cardputer.Display.setTextSize(1.0f);
            M5Cardputer.Display.setCursor(10, 10);
            M5Cardputer.Display.print((std::string("发送失败：") + c).c_str());
            delay(3000);
        }
    }
}

bool UsbService::isReady() const {
    return initialized;
}

void UsbService::sendChunkedString(const std::string& data, size_t chunkSize, unsigned long delayBetweenChunks) {
    if (chunkSize == 0) {
        return;
    }
    size_t totalLength = data.length();
    size_t sentLength = 0;

    while (sentLength < totalLength) {
        size_t remainingLength = totalLength - sentLength;
        size_t currentChunkSize = (remainingLength > chunkSize) ? chunkSize : remainingLength;

        // Extract the current chunk
        std::string chunk = data.substr(sentLength, currentChunkSize);

        // Send the chunk
        sendString(chunk);

        // Delay
        sentLength += currentChunkSize;
        delay(delayBetweenChunks);
    }
}


} // namespace services
