//
// Created by Jakub on 10.03.2024.
//

#include "Logger.h"

LogLevel Logger::currentLevel = LogLevel::Info;

void Logger::log(LogLevel level, const std::string_view &message)  {
    if (level >= currentLevel) {
        switch (level) {
            case LogLevel::Debug:
                std::cout << "[Debug] " << message << std::endl;
                break;
            case LogLevel::Info:
                std::cout << "[Info] " << message << std::endl;
                break;
            case LogLevel::Warning:
                std::cout << "[Warning] " << message << std::endl;
                break;
            case LogLevel::Error:
                std::cout << "[Error] " << message << std::endl;
                break;
            case LogLevel::NoLevel:
                break;
        }
    }
}

void Logger::setLogLevel(LogLevel level)  {
    currentLevel = level;
}
