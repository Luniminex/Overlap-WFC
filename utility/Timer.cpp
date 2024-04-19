//
// Created by Jakub on 10.03.2024.
//


#include "Timer.h"

Timer::Timer(const std::string_view &function_name) {
    this->function_name = function_name;
    Logger::log(LogLevel::Info, "Starting " + std::string(function_name));
    this->start = std::chrono::high_resolution_clock::now();
}

Timer::~Timer() {
    this->end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = this->end - this->start;
    Logger::log(LogLevel::Info, "Finished " + std::string(function_name) + " in " + std::to_string(duration.count()) + "s");
}
