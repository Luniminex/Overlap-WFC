//
// Created by Jakub on 25.05.2024.
//

#ifndef WFC_FILEUTIL_H
#define WFC_FILEUTIL_H

#include <chrono>
#include <string>
#include <filesystem>
#include "Logger.h"

namespace Util {
    class FileUtil {
    public:
        //checks if directory exists, if not creates it, returns true if it exists
        static bool checkDirectory(const std::string &path);

        //checks if the directory of file exists, if not, creates it, returns true if it exists
        static bool checkFileDirectory(const std::string &path);

        //checks if that file name exists, if yes, tries to create a new one with a counter
        static std::string getUniqueFileName(const std::string &path);

        //checks if that directory name exists, if yes, tries to create a new one with a counter
        static std::string checkAndCreateUniqueDirectory(const std::string &baseName);

    private:
    };
}
#endif //WFC_FILEUTIL_H
