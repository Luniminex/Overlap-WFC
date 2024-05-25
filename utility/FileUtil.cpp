//
// Created by Jakub on 25.05.2024.
//

#include "FileUtil.h"

bool Util::FileUtil::checkDirectory(const std::string &path) {
    std::filesystem::path dir(path);
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
        return false;
    }
    return true;
}

bool Util::FileUtil::checkFileDirectory(const std::string &path) {
    std::filesystem::path filePath(path);
    std::filesystem::path dirPath = filePath.parent_path();

    if (!std::filesystem::exists(dirPath)) {
        std::filesystem::create_directories(dirPath);
        return false;
    }
    return true;
}

std::string Util::FileUtil::getUniqueFileName(const std::string &path) {
    std::filesystem::path filePath(path);
    //if file exists, create a new one with a counter
    if (std::filesystem::exists(filePath)) {
        std::string fileName = filePath.stem().string();
        std::string extension = filePath.extension().string();
        std::string parentPath = filePath.parent_path().string();
        std::string newFileName = fileName + "_";
        int counter = 1;
        while (std::filesystem::exists(parentPath + "/" + newFileName + std::to_string(counter) + extension)) {
            counter++;
        }
        return parentPath + "/" + newFileName + std::to_string(counter) + extension;
    }
    return path;
}

std::string Util::FileUtil::checkAndCreateUniqueDirectory(const std::string &baseName) {
    std::filesystem::path dirPath(baseName);
    if (!std::filesystem::exists(dirPath)) {
        std::filesystem::create_directories(dirPath);
        return baseName;
    }

    int counter = 1;
    std::string newDirName;
    do {
        newDirName = baseName + "_" + std::to_string(counter);
        dirPath = newDirName;
        counter++;
    } while (std::filesystem::exists(dirPath));

    std::filesystem::create_directories(dirPath);

    return newDirName;
}
