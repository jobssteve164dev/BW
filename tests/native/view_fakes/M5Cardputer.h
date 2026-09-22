#ifndef TEST_M5CARDPUTER_H
#define TEST_M5CARDPUTER_H

#include <cstdint>
#include <cstdarg>
#include <cstdio>
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

struct RectCall {
    int x;
    int y;
    int width;
    int height;
};

struct LineCall {
    int x1;
    int y1;
    int x2;
    int y2;
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
    int datum;
};

class M5GFX {
public:
    std::vector<CircleCall> filledCircles;
    std::vector<CircleCall> outlinedCircles;
    std::vector<RectCall> drawnRects;
    std::vector<RectCall> filledRects;
    std::vector<LineCall> drawnLines;
    std::vector<QrCodeCall> qrCodes;
    std::vector<TextCall> drawnStrings;
    std::vector<TextCall> printedStrings;
    const fonts::Font* currentFont = nullptr;
    int currentDatum = middle_center;
    int cursorX = 0;
    int cursorY = 0;

    void clearRecordedCalls() {
        filledCircles.clear();
        outlinedCircles.clear();
        drawnRects.clear();
        filledRects.clear();
        drawnLines.clear();
        qrCodes.clear();
        drawnStrings.clear();
        printedStrings.clear();
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
        drawnStrings.push_back({text, x, y, currentFont, currentDatum});
    }

    void setCursor(float x, float y) {
        cursorX = static_cast<int>(x);
        cursorY = static_cast<int>(y);
    }

    void printf(const char* format, ...) {
        char output[256];
        va_list args;
        va_start(args, format);
        std::vsnprintf(output, sizeof(output), format, args);
        va_end(args);
        printedStrings.push_back({output, cursorX, cursorY, currentFont, currentDatum});
    }

    void drawCircle(int x, int y, int radius, uint16_t) {
        outlinedCircles.push_back({x, y, radius});
    }

    void drawRect(int x, int y, int width, int height, uint16_t) {
        drawnRects.push_back({x, y, width, height});
    }

    void drawLine(int x1, int y1, int x2, int y2, uint16_t) {
        drawnLines.push_back({x1, y1, x2, y2});
    }

    void fillRect(int x, int y, int width, int height, uint16_t) {
        filledRects.push_back({x, y, width, height});
    }

    void setTextDatum(int datum) { currentDatum = datum; }

    template <typename... Args> void setRotation(Args...) {}
    template <typename... Args> void setTextColor(Args...) {}
    template <typename... Args> void fillScreen(Args...) {}
    template <typename... Args> void setTextSize(Args...) {}
    template <typename... Args> void drawCenterString(Args...) {}
    template <typename... Args> void drawRoundRect(Args...) {}
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
