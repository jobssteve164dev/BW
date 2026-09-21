#include "SdService.h"

namespace services {

SdService::SdService() {}

bool SdService::begin() {
sdCardSPI.begin(
    globalContext.getSdCardCLKPin(),
    globalContext.getSdCardMISOPin(),
    globalContext.getSdCardMOSIPin(),
    globalContext.getSdCardCSPin()
);
    delay(10);

    if (!SD.begin(globalContext.getSdCardCSPin(), sdCardSPI)) {
        sdCardMounted = false;
        return false;
    }
    
    sdCardMounted = true;
    return sdCardMounted;
}

void SdService::close() {
    SD.end();
    sdCardMounted = false;
}

bool SdService::isFile(const std::string filePath) {
    if (!sdCardMounted) {
        return false;
    }
    File f = SD.open(filePath.c_str());
    if (f && !f.isDirectory()) {
        f.close();
        return true;
    }  
    return false;     
}

bool SdService::getSdState() {
    return sdCardMounted;
}

std::vector<std::string> SdService::listElements(std::string dirPath, size_t limit) {

    if (limit == 0) {
        limit = globalContext.getFileCountLimit(); 
    };

    std::vector<std::string> filesList;
    std::vector<std::string> foldersList;

    if (!sdCardMounted) {
        return filesList;
    }
    
    File dir = SD.open(dirPath.c_str());
    if (!dir) {
        return filesList;
    }

    if (!dir.isDirectory()) {
        return filesList;
    }

    File file = dir.openNextFile();
    if (!file) {
        return filesList;
    }

    size_t i = 0;
    while (file) {
        // Avoid hidden elements
        if (file.name()[0] != '.') {
            if (file.isDirectory()) {
                foldersList.push_back(file.name());
            } else {
                filesList.push_back(file.name());
            }
            i++;
        }

        // limit
        if (i > limit) {
            break;
        }

        file = dir.openNextFile();
    }

    // Sort both folders and files alphabetically
    std::sort(foldersList.begin(), foldersList.end());
    std::sort(filesList.begin(), filesList.end());

    // Merge both
    foldersList.insert(foldersList.end(), filesList.begin(), filesList.end());
    
    return foldersList;

}

std::vector<uint8_t> SdService::readBinaryFile(const char* filePath) {
    std::vector<uint8_t> content;
    if (!sdCardMounted) {
        return content;
    }

    File file = SD.open(filePath, FILE_READ);
    if (file) {
        content.reserve(file.size());
        while (file.available()) {
            content.push_back(file.read());
        }
        file.close();
    }
    return content;
}

std::string SdService::readFile(const char* filePath) {
    std::string content;
    if (!sdCardMounted) {
        return content;
    }

    File file = SD.open(filePath);
    if (file) {
        while (file.available()) {
            content += (char)file.read();
        }
        file.close();
    }
    return content;
}

bool SdService::writeFile(const char* filePath, const std::string& data) {
    if (!sdCardMounted) {
        return false;
    }

    File file = SD.open(filePath, FILE_WRITE);
    if (file) {
        const uint8_t* buffer = reinterpret_cast<const uint8_t*>(data.c_str());
        const size_t written = file.write(buffer, data.length());
        file.close();
        return written == data.length();
    }
    return false;
}

bool SdService::deleteFile(const char* filePath) {
    if (!sdCardMounted) {
        return false;
    }

    if (SD.exists(filePath)) {
        return SD.remove(filePath);
    }
    return false;
}

bool SdService::writeBinaryFile(const char* filePath, const std::vector<uint8_t>& data) {
    if (!sdCardMounted) {
        return false;
    }

    File file = SD.open(filePath, FILE_WRITE);
    if (file) {
        const size_t written = file.write(data.data(), data.size());
        file.close();
        return written == data.size();
    }

    return false; 
}

bool SdService::replaceBinaryFile(const char* filePath,
                                  const char* temporaryPath,
                                  const char* backupPath,
                                  const std::vector<uint8_t>& data) {
    if (!sdCardMounted || data.empty()) {
        return false;
    }

    if (SD.exists(temporaryPath) && !SD.remove(temporaryPath)) {
        return false;
    }
    if (!writeBinaryFile(temporaryPath, data)) {
        return false;
    }
    if (readBinaryFile(temporaryPath) != data) {
        return false;
    }

    const bool hadOriginal = SD.exists(filePath);
    const bool hadBackup = SD.exists(backupPath);
    if (hadOriginal) {
        if (hadBackup && !SD.remove(backupPath)) {
            return false;
        }
        if (!SD.rename(filePath, backupPath)) {
            return false;
        }
    }
    if (!SD.rename(temporaryPath, filePath)) {
        if (hadOriginal) {
            SD.rename(backupPath, filePath);
        }
        return false;
    }

    if (readBinaryFile(filePath) != data) {
        return false;
    }

    // Keep the previous verified generation as a steady-state backup. On the
    // first write, create an identical backup so a later damaged main file can
    // still be recovered.
    if (!hadOriginal && !hadBackup) {
        if (!writeBinaryFile(backupPath, data) || readBinaryFile(backupPath) != data) {
            return false;
        }
    }
    return true;
}

bool SdService::promoteBackupFile(const char* filePath,
                                  const char* backupPath,
                                  const char* corruptPath) {
    if (!sdCardMounted || !SD.exists(backupPath)) {
        return false;
    }

    const bool hadMainFile = SD.exists(filePath);
    if (hadMainFile) {
        if (SD.exists(corruptPath) && !SD.remove(corruptPath)) {
            return false;
        }
        if (!SD.rename(filePath, corruptPath)) {
            return false;
        }
    }

    if (!SD.rename(backupPath, filePath)) {
        if (hadMainFile) {
            SD.rename(corruptPath, filePath);
        }
        return false;
    }
    return true;
}

bool SdService::appendToFile(const char* filePath, const std::string& data) {
    if (!sdCardMounted) {
        return false;
    }

    File file = SD.open(filePath, FILE_APPEND);
    if (file) {
        const uint8_t* buffer = reinterpret_cast<const uint8_t*>(data.c_str());
        file.write(buffer, data.length());
        file.close();
        return true;
    }
    return false;
}

}
