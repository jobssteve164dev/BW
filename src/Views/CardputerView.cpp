#include "CardputerView.h"

namespace views {

M5GFX* CardputerView::Display = nullptr;

void CardputerView::initialize() {
    Display = &M5Cardputer.Display;
    Display->setRotation(1);
    Display->setTextColor(TEXT_COLOR);
    Display->fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.setTextDatum(middle_center);
    M5Cardputer.Display.setFont(&fonts::efontCN_16);
    Display->setTextSize(TEXT_BIG);
}

void CardputerView::displayTopBar(const std::string& title, bool submenu, bool searchBar, bool bitcoinIcon, size_t correctionOffset) {
    uint8_t marginX = 4;
    uint8_t marginY = 14;
    float offsetX; // for text align
    size_t limiter; // char limitation
    float sizeText; // pixels offset depending on text size

    clearTopBar();

    if (submenu) {
        drawSubMenuReturn(marginX+3, marginY); // for return <
        limiter = 20; // limit string size
        sizeText = 5.1; // pixels offset for each char
    } else {
        Display->setTextSize(TEXT_LARGE);
        limiter = 16;
        sizeText = 6.95;
    }

    // To center text
    offsetX = getTextCenterOffset(title, Display->width(), sizeText) - correctionOffset;
    
    if (searchBar) {
        Display->setTextColor(TEXT_COLOR);

        // Empty search query
        const std::string searchQuery = title.empty() ? "输入关键词搜索" : title.substr(0, limiter);
        drawSearchIcon(Display->width() - 20, marginY-2, 10, PRIMARY_COLOR);

        Display->setCursor(offsetX, marginY);
        Display->printf("%s", searchQuery.c_str());
    } else {
        Display->setTextColor(TEXT_COLOR);

        Display->setCursor(offsetX+8, marginY);
        Display->setTextSize(TEXT_BIG);
        Display->printf("%s", title.c_str());
    }

    if (bitcoinIcon) {
        drawBitcoinIcon(offsetX-22, 3);
    }
}


void CardputerView::displaySelection(
                    const std::vector<std::string>& selectionStrings,  
                    uint16_t selectionIndex, const std::vector<std::string>& selectionDescription, 
                    bool upperCase, bool showCurrency){

    uint8_t sizeX = Display->width() - 10; // width of each block
    const bool hasDescriptions = selectionDescription.size() == selectionStrings.size() && !selectionDescription.empty();
    uint8_t sizeY = hasDescriptions ? 44 : 22; // height of each block
    uint8_t startY = 30; // height start block
    uint8_t stepY = hasDescriptions ? 48 : 26; // step between each block
    uint8_t margin = DEFAULT_MARGIN;
    uint8_t startText = hasDescriptions ? 40 : 41; // where text for each block starts
    uint8_t marginText; // width start block
    uint8_t rowsPerScreen = hasDescriptions ? 2 : 4;
    size_t currentIndex; // up to date index
    bool selected; // track selected row
    uint16_t currentStartRow = selectionIndex / rowsPerScreen * rowsPerScreen;
    std::string upperString;

    displayClearMainView();

    // for filtering with no results
    if (selectionStrings.empty()) {
        Display->drawCenterString("没有结果", Display->width() / 2, Display->height() / 2);
    }

    for (size_t i = 0; i < rowsPerScreen && (currentStartRow + i) < selectionStrings.size(); ++i) {
        currentIndex = currentStartRow + i;
        selected = (currentIndex == selectionIndex);
        Display->setTextSize(TEXT_WIDE);
        
        drawRect(selected, margin, startY, sizeX, sizeY, stepY * i);

        // Margin
        marginText = DEFAULT_MARGIN * 2;

        Display->setCursor(marginText + margin, startText + stepY * i);

        // Limiter 
        const char* limiter = "%.17s";

        if (upperCase) {
            upperString = toUpperCase(selectionStrings[currentIndex]);
            Display->printf("%s", upperString.c_str());
        } else { 
            auto truncatedString = truncateString(selectionStrings[currentIndex], 24);
            Display->printf("%s", truncatedString.c_str());
        }

        if (showCurrency) {
            Display->setTextSize(TEXT_WIDE);
            Display->fillRoundRect(Display->width() - 44, startY + stepY * i - 1, 39, sizeY, DEFAULT_ROUND_RECT, RECT_COLOR_DARK);
            Display->drawRoundRect(Display->width() - 45, startY + stepY * i, 40, sizeY, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
            Display->setCursor(Display->width() - 38, startText + stepY * i);
            Display->printf("btc");
        }

        // Description
        if (hasDescriptions) {
            Display->setTextSize(TEXT_TINY);
            const auto& description = selectionDescription[currentIndex];
            auto descriptionX = getTextCenterOffset(description, Display->width(), 0);
            auto descriptionY = startText + 18 + stepY * i;
            Display->setCursor(descriptionX, descriptionY);
            Display->printf("%s", description.c_str());
        }
    }
}

void CardputerView::displayWalletFileInfo(std::string defaultFileName) {
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(52, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("钱包文件");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(34, 46);
    Display->printf("创建钱包后，公开信息");

    // Text
    Display->setCursor(34, 65);
    Display->printf("会保存到 SD 卡");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setTextSize(TEXT_MEDIUM_LARGE);
    auto truncated = truncateString(defaultFileName, 24);
    auto x = getTextCenterOffset(truncated, Display->width(), 4);
    Display->setCursor(x, 88);
    Display->printf("%s", truncated.c_str());

    // Button OK
    Display->fillRoundRect(70, 105, 100, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(80, 115);
    Display->printf("按 OK 继续");
}

void CardputerView::displayStringPrompt(std::string stringDescription,
                                        std::string stringInput,
                                        size_t offsetX,
                                        bool backButton,
                                        bool password,
                                        size_t minimumLength) {
    // Clear
    displayClearMainView(5);

    // Box frame
    Display->drawRoundRect(10, 35, Display->width() - 20, 90, DEFAULT_ROUND_RECT, PRIMARY_COLOR);

    // Description
    Display->setTextSize(TEXT_MEDIUM);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(53-offsetX, 48);
    Display->printf("%s", stringDescription.c_str());

    // Check the length of the input and truncate if necessary
    const std::string visibleInput = password ? std::string(stringInput.length(), '*') : stringInput;
    std::string truncatedInput;
    if (visibleInput.length() > 15) {
        truncatedInput = visibleInput.substr(visibleInput.length() - 15); // Get the last 15 characters
    } else {
        truncatedInput = visibleInput;
    }

    // input
    Display->setTextSize(TEXT_MEDIUM_LARGE);
    Display->drawRoundRect(42, 62, 155, 25, DEFAULT_ROUND_RECT, RECT_COLOR_DARK);
    Display->setCursor(51, 73);
    Display->printf("%s", truncatedInput.c_str());

    size_t xPos = 110; 
    if (backButton) {
        // < button
        Display->drawRoundRect(53, 95, 40, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
        Display->setCursor(70, 103);
        Display->printf("<");
        xPos = 135;
    }

    // Button save
    if (stringInput.length() < minimumLength) {
        Display->drawRoundRect(xPos-30, 95, 80, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    } else {
        Display->fillRoundRect(xPos-30, 95, 80, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    }

    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(xPos, 105);
    Display->printf("OK");
}

void CardputerView::displayConfirmationPrompt(std::string stringDescription) {
    // Clear
    displayClearMainView(5);

    // Box frame
    Display->drawRoundRect(10, 35, Display->width() - 20, 90, DEFAULT_ROUND_RECT, PRIMARY_COLOR);

    // Description
    Display->setTextSize(TEXT_MEDIUM);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(57, 62);
    Display->printf("%s", stringDescription.c_str());
    Display->setTextSize(TEXT_MEDIUM);

    // < button
    Display->drawRoundRect(65, 85, 40, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setCursor(82, 94);
    Display->printf("<");

    // ok button
    Display->fillRoundRect(128, 85, 40, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setCursor(138, 95);
    Display->printf("OK");
}

void CardputerView::displaySubMessage(std::string message, size_t x, int delayMs) {
    // Clear
    displayClearMainView(5);

    // Box frame
    Display->drawRoundRect(10, 35, Display->width() - 20, 90, DEFAULT_ROUND_RECT, PRIMARY_COLOR);

    // Description
    Display->setTextSize(TEXT_WIDE);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(x, 80);
    Display->printf("%s", message.c_str());
    Display->setTextSize(TEXT_MEDIUM);

    if (delayMs) {
        delay(delayMs);
    }
}

void CardputerView::displayMnemonicWord(std::string word, size_t index, size_t size, bool esc, bool restore) {
    // Clear
    displayClearMainView(5);

    // Box frame
    Display->drawRoundRect(10, 35, Display->width() - 20, 90, DEFAULT_ROUND_RECT, PRIMARY_COLOR);

    if (esc) {
        // Esc
        Display->setTextSize(TEXT_TINY);
        Display->setTextColor(PRIMARY_COLOR);
        Display->setCursor(32, 47);
        Display->printf("抄写完成后按 ESC");
    }

    if (restore) {
        Display->setTextSize(TEXT_SMALL);
        Display->setTextColor(PRIMARY_COLOR);
        Display->setCursor(24, 49);
        Display->printf("输入单词后按 OK");
    }


    // Word box
    Display->drawRoundRect(40, 65, Display->width() - 77, 35, DEFAULT_ROUND_RECT, RECT_COLOR_LIGHT);
    
    // Word
    Display->setTextSize(TEXT_BIG);
    Display->setTextColor(TEXT_COLOR);
    auto offsetX= getTextCenterOffset(word, Display->width(), 8);
    Display->setCursor(offsetX, 80);
    Display->printf("%s", word.c_str());

    if (!restore) {
        // < >
        Display->setCursor(20, 80);
        Display->printf("<");
        Display->setCursor(Display->width() - 30, 80);
        Display->printf(">");
    }

    // Counter
    Display->setTextColor(PRIMARY_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(Display->width() / 2 - 15, Display->height() - 23);
    Display->printf("%d/%d", index+1, size);
    Display->setTextColor(TEXT_COLOR);
}

void CardputerView::drawSearchIcon(int x, int y, int size, uint16_t color) {
    int radius = size / 2;
    int handleLength = size / 2;

    // Dessiner cercle
    Display->drawCircle(x, y, radius, color);
    // Dessiner poignée
    Display->drawLine(x + radius, y + radius, x + radius + handleLength, y + radius + handleLength, color);
}

void CardputerView::drawRect(bool selected, uint8_t margin, uint16_t startY, uint16_t sizeX, uint16_t sizeY, uint16_t stepY) {
        // Draw rect
        if (selected) {
            Display->fillRoundRect(margin, startY + stepY, sizeX, sizeY, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
            Display->setTextColor(TEXT_COLOR);
        } else {
            Display->fillRoundRect(margin, startY + stepY , sizeX, sizeY, DEFAULT_ROUND_RECT, RECT_COLOR_DARK);
            Display->drawRoundRect(margin, startY + stepY, sizeX, sizeY, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
            Display->setTextColor(TEXT_COLOR);
        }
}

void CardputerView::drawSubMenuReturn(uint8_t x, uint8_t y) {
    Display->setTextSize(TEXT_WIDE);
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(x, y);
    Display->printf("<");
}

void CardputerView::displayClearMainView(uint8_t offsetY) {
    Display->fillRect(0, TOP_BAR_HEIGHT-offsetY, Display->width(), Display->height(), BACKGROUND_COLOR);
}

void CardputerView::clearTopBar() {
    Display->fillRect(0, 0, Display->width(), TOP_BAR_HEIGHT, BACKGROUND_COLOR);
}

void CardputerView::drawBitcoinIcon(int x, int y) {
    int radius = 22 / 2;

    // Dessiner le cercle
    Display->fillCircle(x + radius, y + radius, radius, PRIMARY_COLOR);
    Display->drawCircle(x + radius, y + radius, radius, BACKGROUND_COLOR);

    // Dessiner le "B"
    int innerRadius = radius / 2;
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_TINY);
    Display->setTextDatum(middle_center);
    M5Cardputer.Display.setFont(&fonts::FreeSerifBold24pt7b);
    Display->drawString("B", x + radius, y + radius + 1);
    M5Cardputer.Display.setFont(&fonts::efontCN_16);
}

void CardputerView::displayDebug(std::string message) {
    Display->setTextSize(TEXT_MEDIUM);
    Display->fillScreen(TFT_BLACK);
    Display->setCursor(100, 10);
    Display->printf("调试信息");
    Display->setCursor(10, 50);
    Display->printf("%s", message.c_str());
    delay(3000);
}

float CardputerView::getTextCenterOffset(const std::string& text, int16_t width, float sizeText) {
    (void)sizeText;
    return (width - Display->textWidth(text.c_str())) / 2.0f;
}

std::string CardputerView::fitTextToWidth(const std::string& text, int16_t maxWidth) {
    if (Display->textWidth(text.c_str()) <= maxWidth) return text;
    std::string fitted = text;
    while (!fitted.empty() && Display->textWidth((fitted + "...").c_str()) > maxWidth) fitted.pop_back();
    return fitted + "...";
}

void CardputerView::displayKeyboardLayout(const std::string& layoutName) {
    // Clear
    displayClearMainView(5);

    // Box frame
    Display->drawRoundRect(10, 35, Display->width() - 20, 90, DEFAULT_ROUND_RECT, PRIMARY_COLOR);

    // Infos
    Display->setTextSize(TEXT_TINY);
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(17, 45);
    Display->printf("按 OK 选择键盘布局");
    Display->setCursor(17, 113);
    Display->printf("用于通过 USB 输入比特币地址");

    // Layout name box
    Display->drawRoundRect(40, 63, Display->width() - 77, 35, DEFAULT_ROUND_RECT, RECT_COLOR_LIGHT);
    
    // Layout name
    Display->setTextSize(TEXT_WIDE);
    Display->setTextColor(TEXT_COLOR);
    auto offsetX = getTextCenterOffset(layoutName, Display->width(), 4.5);
    Display->setCursor(offsetX, 80);
    Display->printf("%s", layoutName.c_str());

    // Arrows for navigation
    Display->setCursor(20, 78);
    Display->printf("<");   
    Display->setCursor(Display->width() - 26, 78);
    Display->printf(">");

    // Reset text color to default
    Display->setTextColor(PRIMARY_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
}

void CardputerView::displayWalletValue(std::string description, std::string value) {
    // Clear the main view area
    displayClearMainView(5);

    // Display value
    Display->setTextSize(TEXT_BIG);
    Display->setTextColor(TEXT_COLOR);
    Display->setFont(&fonts::FreeSans12pt7b);
    Display->setTextWrap(false);
    auto preview = fitTextToWidth(value, Display->width() - 12);
    auto previewX = getTextCenterOffset(preview, Display->width(), 0);
    Display->setCursor(previewX, 62);
    Display->printf("%s", preview.c_str());
    Display->setTextWrap(true);
    Display->setFont(&fonts::efontCN_16);

    // Display "Q" button
    Display->setTextSize(TEXT_MEDIUM);
    Display->fillRoundRect(40, 95, 20, 15, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(45, 101);
    Display->printf("q");

    Display->setCursor(68, 102);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("显示二维码");

    // Display "OK" button
    Display->fillRoundRect(40, 117, 30, 15, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(47, 124);
    Display->printf("ok");

    // Display->setTextSize(TEXT_TINY);
    Display->setCursor(80, 123);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("通过 USB 输入");

    Display->setTextColor(TEXT_COLOR);
}

void CardputerView::displayPlugUsbMention() {
    Display->setTextSize(TEXT_MEDIUM);
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(30, 120);
    Display->printf("请连接 USB 键盘模式");
}


void CardputerView::displayQrCode(std::string address) {
    Display->fillScreen(BACKGROUND_COLOR);
    M5Cardputer.Display.qrcode(address.c_str(), -1, -1, 125);
}

void CardputerView::setBrightness(uint16_t brightness) {
    Display->setBrightness(brightness);
}

std::string CardputerView::toUpperCase(const std::string& text) {
    std::string uppercaseText = text;
    std::transform(uppercaseText.begin(), uppercaseText.end(), uppercaseText.begin(), ::toupper);
    return uppercaseText;
}

void CardputerView::displayTopIcon() {
    drawBitcoinIcon(207, 7);
    drawSubMenuReturn(10, 15);
}

void CardputerView::displaySeedStart(){
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(40, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("关于助记词");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(30, 46);
    Display->printf("即将创建新助记词");

    // Text
    Display->setCursor(20, 65);
    Display->printf("请把 24 个单词抄到纸上");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(12, 88);
    Display->setTextSize(TEXT_WIDE);
    Display->printf("丢失助记词等于丢失钱包");

    // Button OK
    Display->fillRoundRect(70, 105, 100, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(80, 115);
    Display->printf("按 OK 开始");
}

void CardputerView::displaySeedRfid(){
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(45, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("关于 RFID");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(12, 46);
    Display->printf("可将助记词保存到标签");

    // Text
    Display->setCursor(20, 65);
    Display->printf("如需使用，请连接 RFID2");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(8, 88);
    Display->setTextSize(TEXT_SMALL);
    Display->printf("可使用密码加密备份");

    // Button Next
    Display->fillRoundRect(80, 105, 80, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(90, 115);
    Display->printf("下一步 ->");
}

void CardputerView::displaySeedEnd(bool sdCardMount, bool vaultSaved) {
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(33, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("钱包已创建");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(41, 46);
    Display->printf("%s", vaultSaved ? "加密备份已保存" : "请妥善保管助记词");

    // Text
    Display->setCursor(12, 65);
    Display->printf("%s", sdCardMount ? "公开信息已保存到 SD 卡" : "公开信息未保存到 SD 卡");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setTextSize(TEXT_MEDIUM_LARGE);
    auto finalString = vaultSaved ? "/bw-vault.dat" :
                       (sdCardMount ? "/card-wallets.txt" : "重启后将丢失");
    auto offsetX = getTextCenterOffset(finalString, Display->width(), 4);
    Display->setCursor(offsetX, 88);
    Display->printf("%s", finalString);

    // Button OK
    Display->fillRoundRect(70, 105, 100, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(80, 115);
    Display->printf("按 OK 继续");
}

void CardputerView::displayPlugRfid(){
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(54, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("连接 RFID");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(42, 46);
    Display->printf("请连接 RFID2 模块");

    // Text
    Display->setCursor(45, 65);
    Display->printf("插入 I2C Grove 接口");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(20, 88);
    Display->setTextSize(TEXT_SMALL);
    Display->printf("准备好后按 OK");

    // Button Next
    Display->fillRoundRect(80, 105, 80, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(90, 115);
    Display->printf("下一步 ->");
}

void CardputerView::displaySeedGeneralInfos() {
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(40, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("关于助记词");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(25, 46);
    Display->printf("助记词默认不会保存");

    // Text
    Display->setCursor(30, 65);
    Display->printf("如果没有备份或已经丢失");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(13, 88);
    Display->setTextSize(TEXT_MEDIUM);
    Display->printf("将无法找回钱包资金");

    // Button Next
    Display->fillRoundRect(80, 105, 80, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(90, 115);
    Display->printf("下一步 ->");
}

void CardputerView::displayRfidInfos() {
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(43, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("关于 RFID");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(17, 46);
    Display->printf("可将助记词保存到标签");

    // Text
    Display->setCursor(17, 65);
    Display->printf("签名交易时可直接读取");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(8, 88);
    Display->setTextSize(TEXT_MEDIUM);
    Display->printf("需要 M5Stack RFID2 模块");

    // Button Next
    Display->fillRoundRect(80, 105, 80, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(90, 115);
    Display->printf("下一步 ->");
}

void CardputerView::displayRfidTagInfos() {
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(50, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("关于标签");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(14, 46);
    Display->printf("需要空白 MIFARE 1K 标签");

    // Text
    Display->setCursor(9, 65);
    Display->printf("可明文或加密保存");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(15, 88);
    Display->setTextSize(TEXT_MEDIUM);
    Display->printf("写入前标签必须为空");

    // Button Next
    Display->fillRoundRect(80, 105, 80, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(90, 115);
    Display->printf("下一步 ->");
}

void CardputerView::displaySeedLoadInfos() {
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(46, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("加载助记词");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(15, 46);
    Display->printf("可从加密保险库解锁");

    // Text
    Display->setCursor(20, 65);
    Display->printf("签名时输入保险库密码");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(24, 88);
    Display->setTextSize(TEXT_MEDIUM);
    Display->printf("也可用 RFID 或手动输入");

    // Button Next
    Display->fillRoundRect(80, 105, 80, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(90, 115);
    Display->printf("下一步 ->");
}

void CardputerView::displaySeedFormatGeneralInfos() {
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(25, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("助记词格式");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(30, 46);
    Display->printf("助记词遵循 BIP39 标准");

    // Text
    Display->setCursor(25, 65);
    Display->printf("可在兼容钱包中恢复");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(16, 88);
    Display->setTextSize(TEXT_MEDIUM);
    Display->printf("地址使用 SegWit BIP84");

    // Button Next
    Display->fillRoundRect(80, 105, 80, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(90, 115);
    Display->printf("下一步 ->");
}

void CardputerView::displaySdSaveGeneralInfos() {
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(20, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("关于 SD 卡");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(28, 46);
    Display->printf("公开信息会自动加载");

    // Text
    Display->setCursor(22, 65);
    Display->printf("可选加密保存助记词");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(26, 88);
    Display->setTextSize(TEXT_MEDIUM_LARGE);
    Display->printf("签名时输入密码解锁");

    // Button Next
    Display->fillRoundRect(80, 105, 80, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(90, 115);
    Display->printf("下一步 ->");
}

void CardputerView::displaySeedRestorationInfos() {
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(40, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("恢复助记词");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(22, 46);
    Display->printf("请逐个输入助记词");

    // Text
    Display->setCursor(27, 65);
    Display->printf("请准确输入每个英文单词");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(28, 88);
    Display->setTextSize(TEXT_MEDIUM);
    Display->printf("完成后即可恢复钱包");

    // Button Next
    Display->fillRoundRect(80, 105, 80, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(90, 115);
    Display->printf("下一步 ->");
}

void CardputerView::displayFileVersionInfos() {
    Display->fillScreen(BACKGROUND_COLOR);

    // Box frame
    Display->drawRect(1, 1, Display->width() - 1, Display->height() - 1, PRIMARY_COLOR);

    // Main title
    Display->setTextSize(TEXT_BIG);
    Display->setCursor(36, 22);
    Display->setTextColor(PRIMARY_COLOR);
    Display->printf("文件版本");

    // Sub title
    Display->setTextSize(TEXT_SMALL);
    Display->setTextColor(TEXT_COLOR);
    Display->setCursor(32, 46);
    Display->printf("不支持版本 1 钱包文件");

    // Text
    Display->setCursor(16, 65);
    Display->printf("请用助记词恢复钱包");
    Display->setTextColor(PRIMARY_COLOR);
    Display->setCursor(28, 88);
    Display->setTextSize(TEXT_MEDIUM);
    Display->printf("恢复后将生成新版文件");

    // Button Next
    Display->fillRoundRect(80, 105, 80, 20, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    Display->setTextColor(TEXT_COLOR);
    Display->setTextSize(TEXT_MEDIUM);
    Display->setCursor(90, 115);
    Display->printf("下一步 ->");
}

std::string CardputerView::truncateString(const std::string& input, size_t maxLength) {
    const std::string ellipsis = "...";

    if (input.length() <= maxLength) {
        return input;
    }

    // Calcul char number each side
    size_t halfLength = (maxLength - ellipsis.length()) / 2;

    // Start of the fist string, end of the second
    std::string firstPart = input.substr(0, halfLength);
    std::string secondPart = input.substr(input.length() - halfLength);

    // Concat with "..."
    return firstPart + ellipsis + secondPart;
}

}
