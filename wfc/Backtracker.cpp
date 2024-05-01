//
// Created by Jakub on 24.04.2024.
//

#include "Backtracker.h"

Backtracker::Backtracker() {
    options = {0, 0, false};
    lastIteration = 0;
    states = std::deque<std::pair<State, size_t>>();
}

Backtracker::Backtracker(BacktrackerOptions options) : options(options) {
    lastIteration = 0;
    backtracking = false;
    states = std::deque<std::pair<State, size_t>>();
}

void Backtracker::push(const State &state) {
    states.emplace_front(state, options.maxIterations);
    if (states.size() > options.maxDepth) {
        states.pop_back();
    }
}

State Backtracker::draw() {
    Logger::log(LogLevel::Info, "States left: " + std::to_string(states.size()));
    if (states.empty()) {
        return {};
    }
    if (states.front().second > 0) {
        states.front().second--;
        return states.front().first;
    }
    states.pop_front();
    if (states.empty()) {
        return {};
    }
    return states.front().first;
}

bool Backtracker::isEnabled() const {
    return options.enabled;
}

void Backtracker::setEnabled(bool enabled) {
    options.enabled = enabled;
}

void Backtracker::setOptions(const BacktrackerOptions &options) {
    this->options = options;
}

bool Backtracker::isAbleToBacktrack() const {
    return !states.empty();
}

size_t Backtracker::getLastIteration() const {
    return lastIteration;
}

void Backtracker::setBacktracking(bool backtracking) {
    this->backtracking = backtracking;
}

bool Backtracker::isBacktracking() const {
    if (options.enabled) {
        return backtracking;
    }
    return false;
}

void Backtracker::pushBacktrackedState(const State &state) {
    backtrackedStates.push_front(state);
    if (backtrackedStates.size() > options.maxDepth) {
        backtrackedStates.pop_back();
    }
}

void Backtracker::mergeBacktrackedStates() {
    if (backtrackedStates.empty()) {
        return;
    }
    for (auto &state : backtrackedStates) {
        push(state);
    }
    backtrackedStates.clear();
}

void Backtracker::setLastIteration(size_t lastIteration) {
    this->lastIteration = lastIteration;
}

void Backtracker::logStates() const {
    for (const auto &state: states) {
        for (const auto &row: state.first.state) {
            for (const auto &cell: row) {
                std::cout << cell.size() << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}

