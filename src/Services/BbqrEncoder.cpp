#include "BbqrEncoder.h"

#include <algorithm>

namespace services {
namespace {

const char HEX[] = "0123456789ABCDEF";
const size_t MAX_PARTS = 36 * 36 - 1;

} // namespace

std::string BbqrEncoder::base36(size_t value) {
    const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string encoded(2, '0');
    encoded[0] = digits[(value / 36) % 36];
    encoded[1] = digits[value % 36];
    return encoded;
}

bool BbqrEncoder::encodePsbt(const std::vector<uint8_t>& data,
                             std::vector<std::string>& frames,
                             size_t maximumChunkBytes) {
    frames.clear();
    if (data.empty() || maximumChunkBytes == 0) {
        return false;
    }

    const size_t partCount = (data.size() + maximumChunkBytes - 1) / maximumChunkBytes;
    if (partCount == 0 || partCount > MAX_PARTS) {
        return false;
    }

    const size_t chunkBytes = (data.size() + partCount - 1) / partCount;
    frames.reserve(partCount);
    for (size_t part = 0; part < partCount; ++part) {
        const size_t begin = part * chunkBytes;
        const size_t end = std::min(begin + chunkBytes, data.size());
        std::string frame = "B$HP" + base36(partCount) + base36(part);
        frame.reserve(8 + (end - begin) * 2);
        for (size_t index = begin; index < end; ++index) {
            frame.push_back(HEX[data[index] >> 4]);
            frame.push_back(HEX[data[index] & 0x0f]);
        }
        frames.push_back(frame);
    }
    return true;
}

} // namespace services
