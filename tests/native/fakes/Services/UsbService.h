#ifndef TEST_USB_SERVICE_H
#define TEST_USB_SERVICE_H

#include <cstdint>
#include <string>
#include <vector>

inline void delay(unsigned long) {}

namespace services {

class UsbService {
public:
    explicit UsbService(std::vector<std::string>& trace) : trace(trace) {}

    void setLayout(const uint8_t*) { trace.push_back("setLayout"); }
    void begin() { trace.push_back("begin"); }
    void sendString(const std::string&) { trace.push_back("send"); }

private:
    std::vector<std::string>& trace;
};

} // namespace services

#endif // TEST_USB_SERVICE_H
