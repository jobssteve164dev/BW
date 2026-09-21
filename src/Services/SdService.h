#ifndef SD_SERVICE_H
#define SD_SERVICE_H

#include <SD.h>
#include <SPI.h>
#include <vector>
#include <string>
#include <Contexts/GlobalContext.h>
#include <M5Cardputer.h>
#include <Services/BoundedFileReader.h>

using namespace contexts;

namespace services {


class SdService {
public:
    static constexpr size_t MAX_TEXT_FILE_SIZE = 32768;
    static constexpr size_t MAX_BINARY_FILE_SIZE = 131072;

    SdService();
    bool begin(); 
    void close();
    bool isFile(std::string filePath);
    bool getSdState();

    std::vector<std::string> listElements(std::string dirPath, size_t limit=0);
    std::string readFile(const char* filePath, size_t maximumSize = MAX_TEXT_FILE_SIZE);
    bool writeFile(const char* filePath, const std::string& data);
    bool writeBinaryFile(const char* filePath, const std::vector<uint8_t>& data);
    bool replaceBinaryFile(const char* filePath,
                           const char* temporaryPath,
                           const char* backupPath,
                           const std::vector<uint8_t>& data);
    bool promoteBackupFile(const char* filePath,
                           const char* backupPath,
                           const char* corruptPath);
    bool appendToFile(const char* filePath, const std::string& data);
    std::vector<uint8_t> readBinaryFile(const char* filePath,
                                        size_t maximumSize = MAX_BINARY_FILE_SIZE);
    bool deleteFile(const char* filePath);
private:
    GlobalContext& globalContext = GlobalContext::getInstance();
    SPIClass sdCardSPI;
    bool sdCardMounted = false; 
};

}
#endif // SD_SERVICE_H
