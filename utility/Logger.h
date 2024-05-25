//
// Created by Jakub on 10.03.2024.
//

#ifndef WFC_LOGGER_H
#define WFC_LOGGER_H

#include <iostream>

namespace Util {
    enum class LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Important = 4,
        Silent = 5
    };

    class Logger {
    public:

        static void log(LogLevel level, const std::string_view &message);

        static void setLogLevel(LogLevel level);

    private:
        Logger() = default;

        ~Logger() = default;

        static LogLevel currentLevel;
    };
}

#endif //WFC_LOGGER_H
