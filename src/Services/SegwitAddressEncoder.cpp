#include "SegwitAddressEncoder.h"

namespace services {
namespace {

const char CHARSET[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
const uint32_t BECH32_CONSTANT = 1;
const uint32_t BECH32M_CONSTANT = 0x2bc830a3;

uint32_t polymod(const std::vector<uint8_t>& values) {
    const uint32_t generators[] = {
        0x3b6a57b2,
        0x26508e6d,
        0x1ea119fa,
        0x3d4233dd,
        0x2a1462b3,
    };
    uint32_t checksum = 1;
    for (const auto value : values) {
        const uint8_t top = static_cast<uint8_t>(checksum >> 25);
        checksum = ((checksum & 0x1ffffff) << 5) ^ value;
        for (size_t index = 0; index < 5; ++index) {
            if ((top >> index) & 1) {
                checksum ^= generators[index];
            }
        }
    }
    return checksum;
}

std::vector<uint8_t> expandHrp(const std::string& hrp) {
    std::vector<uint8_t> expanded;
    expanded.reserve(hrp.size() * 2 + 1);
    for (const auto character : hrp) {
        expanded.push_back(static_cast<uint8_t>(character >> 5));
    }
    expanded.push_back(0);
    for (const auto character : hrp) {
        expanded.push_back(static_cast<uint8_t>(character & 31));
    }
    return expanded;
}

bool convertEightToFive(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    uint32_t accumulator = 0;
    uint8_t bits = 0;
    output.clear();
    output.reserve((input.size() * 8 + 4) / 5);
    for (const auto value : input) {
        accumulator = (accumulator << 8) | value;
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            output.push_back(static_cast<uint8_t>((accumulator >> bits) & 31));
        }
    }
    if (bits > 0) {
        output.push_back(static_cast<uint8_t>((accumulator << (5 - bits)) & 31));
    }
    return true;
}

} // namespace

std::string SegwitAddressEncoder::encode(const std::string& humanReadablePart,
                                         uint8_t witnessVersion,
                                         const std::vector<uint8_t>& witnessProgram) {
    if (humanReadablePart.empty() || witnessVersion > 16 ||
        witnessProgram.size() < 2 || witnessProgram.size() > 40 ||
        (witnessVersion == 0 && witnessProgram.size() != 20 && witnessProgram.size() != 32)) {
        return "";
    }
    for (const auto character : humanReadablePart) {
        if (character < 33 || character > 126 || (character >= 'A' && character <= 'Z')) {
            return "";
        }
    }

    std::vector<uint8_t> data(1, witnessVersion);
    std::vector<uint8_t> converted;
    if (!convertEightToFive(witnessProgram, converted)) {
        return "";
    }
    data.insert(data.end(), converted.begin(), converted.end());

    auto checksumInput = expandHrp(humanReadablePart);
    checksumInput.insert(checksumInput.end(), data.begin(), data.end());
    checksumInput.insert(checksumInput.end(), 6, 0);
    const uint32_t constant = witnessVersion == 0 ? BECH32_CONSTANT : BECH32M_CONSTANT;
    const uint32_t checksum = polymod(checksumInput) ^ constant;

    std::string address = humanReadablePart + '1';
    for (const auto value : data) {
        address.push_back(CHARSET[value]);
    }
    for (size_t index = 0; index < 6; ++index) {
        address.push_back(CHARSET[(checksum >> (5 * (5 - index))) & 31]);
    }
    return address.size() <= 90 ? address : "";
}

} // namespace services
