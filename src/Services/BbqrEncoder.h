#ifndef BBQR_ENCODER_H
#define BBQR_ENCODER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace services {

class BbqrEncoder {
public:
    static bool encodePsbt(const std::vector<uint8_t>& data,
                           std::vector<std::string>& frames,
                           size_t maximumChunkBytes = 100);

private:
    static std::string base36(size_t value);
};

} // namespace services

#endif // BBQR_ENCODER_H
