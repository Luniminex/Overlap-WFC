//
// Created by Jakub on 10.03.2024.
//

#ifndef WFC_TIMER_H
#define WFC_TIMER_H

#include <string_view>
#include <chrono>
#include "Logger.h"

class Timer {
public:
    Timer(const std::string_view& function_name);
    ~Timer();
private:
    std::string function_name;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
};


#endif //WFC_TIMER_H
