//
// Created by Jakub on 10.03.2024.
//


#include "Timer.h"

Util::Timer::Timer(const std::string_view &function_name) {
    this->function_name = function_name;
    Logger::log(LogLevel::Info, "Starting " + std::string(function_name));
    this->start = std::chrono::high_resolution_clock::now();
}

Util::Timer::~Timer() {
    this->end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = this->end - this->start;
    Logger::log(LogLevel::Info, "Finished " + std::string(function_name) + " in " + std::to_string(duration.count()) + "s");
}

std::chrono::duration<double> Util::Timer::getCurrent() {
    this->end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = this->end - this->start;
    Logger::log(LogLevel::Info,
                "Current time of " + std::string(function_name) + " is " + std::to_string(duration.count()) + "s");
    return duration;
}
