#ifndef WALLET_VALUE_VIEWPORT_H
#define WALLET_VALUE_VIEWPORT_H

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace services {

class WalletValueViewport {
public:
    WalletValueViewport(const std::string& value, size_t charactersPerLine, size_t visibleLineCount)
        : visibleLineCount(std::max<size_t>(1, visibleLineCount)) {
        const size_t lineLength = std::max<size_t>(1, charactersPerLine);
        if (value.empty()) {
            lines.push_back("");
            return;
        }

        for (size_t offset = 0; offset < value.size(); offset += lineLength) {
            lines.push_back(value.substr(offset, lineLength));
        }
    }

    bool scrollUp() {
        if (!canScrollUp()) return false;
        --firstLine;
        return true;
    }

    bool scrollDown() {
        if (!canScrollDown()) return false;
        ++firstLine;
        return true;
    }

    bool canScrollUp() const {
        return firstLine > 0;
    }

    bool canScrollDown() const {
        return firstLine + visibleLineCount < lines.size();
    }

    size_t firstVisibleLine() const {
        return firstLine;
    }

    const std::vector<std::string>& allLines() const {
        return lines;
    }

    std::vector<std::string> visibleLines() const {
        const size_t end = std::min(lines.size(), firstLine + visibleLineCount);
        return std::vector<std::string>(lines.begin() + firstLine, lines.begin() + end);
    }

private:
    std::vector<std::string> lines;
    size_t visibleLineCount;
    size_t firstLine = 0;
};

} // namespace services

#endif // WALLET_VALUE_VIEWPORT_H
