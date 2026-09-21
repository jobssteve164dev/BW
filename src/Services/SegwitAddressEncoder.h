#ifndef SEGWIT_ADDRESS_ENCODER_H
#define SEGWIT_ADDRESS_ENCODER_H

#include <cstdint>
#include <string>
#include <vector>

namespace services {

class SegwitAddressEncoder {
public:
    static std::string encode(const std::string& humanReadablePart,
                              uint8_t witnessVersion,
                              const std::vector<uint8_t>& witnessProgram);
};

} // namespace services

#endif // SEGWIT_ADDRESS_ENCODER_H
