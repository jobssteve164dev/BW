#ifndef BOUNDED_FILE_READER_H
#define BOUNDED_FILE_READER_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace services {

class BoundedFileReader {
public:
    template <typename Reader>
    static bool read(Reader& reader,
                     size_t declaredSize,
                     size_t maximumSize,
                     std::vector<uint8_t>& output) {
        output.clear();
        if (declaredSize > maximumSize) {
            return false;
        }

        output.reserve(declaredSize);
        while (reader.available()) {
            if (output.size() >= declaredSize || output.size() >= maximumSize) {
                output.clear();
                return false;
            }
            const int value = reader.read();
            if (value < 0 || value > 0xff) {
                output.clear();
                return false;
            }
            output.push_back(static_cast<uint8_t>(value));
        }

        if (output.size() != declaredSize) {
            output.clear();
            return false;
        }
        return true;
    }
};

} // namespace services

#endif // BOUNDED_FILE_READER_H
