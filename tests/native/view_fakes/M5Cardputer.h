#ifndef TEST_M5CARDPUTER_H
#define TEST_M5CARDPUTER_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#define TFT_BLACK 0

constexpr int middle_center = 0;

namespace fonts {
struct Font {};
extern Font efontCN_16;
extern Font FreeSerifBold24pt7b;
extern Font Font0;
} // namespace fonts

struct CircleCall {
    int x;
    int y;
    int radius;
};

struct QrCodeCall {
    int x;
    int y;
    int size;
};

struct TextCall {
    std::string text;
    int x;
    int y;
    const fonts::Font* font;
};

class M5GFX {
public:
    std::vector<CircleCall> filledCircles;
    std::vector<QrCodeCall> qrCodes;
    std::vector<TextCall> drawnStrings;
    const fonts::Font* currentFont = nullptr;

    void clearRecordedCalls() {
        filledCircles.clear();
        qrCodes.clear();
        drawnStrings.clear();
    }

    int width() const { return 240; }
    int height() const { return 135; }
    int textWidth(const char* text) const { return static_cast<int>(std::strlen(text)) * 8; }

    void fillCircle(int x, int y, int radius, uint16_t) {
        filledCircles.push_back({x, y, radius});
    }

    void qrcode(const char*, int x, int y, int size) {
        qrCodes.push_back({x, y, size});
    }

    void setFont(const fonts::Font* font) { currentFont = font; }

    void drawString(const char* text, int x, int y) {
        drawnStrings.push_back({text, x, y, currentFont});
    }

    template <typename... Args> void setRotation(Args...) {}
    template <typename... Args> void setTextColor(Args...) {}
    template <typename... Args> void fillScreen(Args...) {}
    template <typename... Args> void setTextDatum(Args...) {}
    template <typename... Args> void setTextSize(Args...) {}
    template <typename... Args> void setCursor(Args...) {}
    template <typename... Args> void printf(Args...) {}
    template <typename... Args> void drawCenterString(Args...) {}
    template <typename... Args> void drawCircle(Args...) {}
    template <typename... Args> void drawLine(Args...) {}
    template <typename... Args> void drawRect(Args...) {}
    template <typename... Args> void drawRoundRect(Args...) {}
    template <typename... Args> void fillRect(Args...) {}
    template <typename... Args> void fillRoundRect(Args...) {}
    template <typename... Args> void setBrightness(Args...) {}
    template <typename... Args> void setTextWrap(Args...) {}
};

class PowerFake {
public:
    int getBatteryLevel() const { return 100; }
};

struct M5CardputerClass {
    M5GFX Display;
    PowerFake Power;
};

extern M5CardputerClass M5Cardputer;

inline void delay(int) {}

#endif
