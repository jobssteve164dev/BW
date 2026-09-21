#ifndef WALLET_VALUE_LAYOUT_H
#define WALLET_VALUE_LAYOUT_H

namespace services {

struct WalletValueLayout {
    enum {
        SCREEN_WIDTH = 240,
        SCREEN_HEIGHT = 135,
        ASCII_GLYPH_WIDTH = 8,
        FONT_HEIGHT = 16,
        CONTENT_WIDTH = 208,
        CHARACTERS_PER_LINE = CONTENT_WIDTH / ASCII_GLYPH_WIDTH,
        VISIBLE_LINE_COUNT = 3,
        CONTENT_CENTER_X = 112,
        INDICATOR_CENTER_X = 230,
        FIRST_LINE_CENTER_Y = 41,
        LINE_SPACING = FONT_HEIGHT + 4,
        ACTION_AREA_TOP = 95,
        ACTION_ROW_SPACING = 22,
        ACTION_HEIGHT = 15
    };
};

} // namespace services

#endif // WALLET_VALUE_LAYOUT_H
