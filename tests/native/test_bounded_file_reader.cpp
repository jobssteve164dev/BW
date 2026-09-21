#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "Services/BoundedFileReader.h"

namespace {

class FakeReader {
public:
    explicit FakeReader(std::vector<int> bytes) : bytes(std::move(bytes)) {}

    int available() const {
        return position < bytes.size();
    }

    int read() {
        ++readCalls;
        return position < bytes.size() ? bytes[position++] : -1;
    }

    size_t readCalls = 0;

private:
    std::vector<int> bytes;
    size_t position = 0;
};

void testReadsDeclaredContentWithinLimit() {
    FakeReader reader({'p', 's', 'b', 't'});
    std::vector<uint8_t> output;
    assert(services::BoundedFileReader::read(reader, 4, 8, output));
    assert(output == std::vector<uint8_t>({'p', 's', 'b', 't'}));
}

void testRejectsOversizedFileBeforeReading() {
    FakeReader reader({'x'});
    std::vector<uint8_t> output = {0xaa};
    assert(!services::BoundedFileReader::read(reader, 9, 8, output));
    assert(output.empty());
    assert(reader.readCalls == 0);
}

void testRejectsStreamThatExceedsDeclaredSize() {
    FakeReader reader({'a', 'b', 'c'});
    std::vector<uint8_t> output;
    assert(!services::BoundedFileReader::read(reader, 2, 8, output));
    assert(output.empty());
}

void testRejectsShortOrFailedRead() {
    FakeReader shortReader({'a'});
    std::vector<uint8_t> output;
    assert(!services::BoundedFileReader::read(shortReader, 2, 8, output));
    assert(output.empty());

    FakeReader failedReader({-1});
    assert(!services::BoundedFileReader::read(failedReader, 1, 8, output));
    assert(output.empty());
}

} // namespace

int main() {
    testReadsDeclaredContentWithinLimit();
    testRejectsOversizedFileBeforeReading();
    testRejectsStreamThatExceedsDeclaredSize();
    testRejectsShortOrFailedRead();
    std::cout << "bounded file reader tests passed\n";
    return 0;
}
