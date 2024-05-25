//
// Created by Jakub on 24.04.2024.
//

#include "Backtracker.h"

WFC::Backtracker::Backtracker() {
    options = {0, 0, false};
    lastIteration = 0;
    states = std::deque<std::pair<State, size_t>>();
}

WFC::Backtracker::Backtracker(BacktrackerOptions options) : options(options) {
    lastIteration = 0;
    backtracking = false;
    states = std::deque<std::pair<State, size_t>>();
}

void WFC::Backtracker::push(const State &state) {
    states.emplace_front(state, options.maxIterations);
    if (states.size() > options.maxDepth) {
        states.pop_back();
    }
}

WFC::State WFC::Backtracker::draw() {
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

bool WFC::Backtracker::isEnabled() const {
    return options.enabled;
}

void WFC::Backtracker::setEnabled(bool enabled) {
    options.enabled = enabled;
}

void WFC::Backtracker::setOptions(const BacktrackerOptions &options) {
    this->options = options;
}

bool WFC::Backtracker::isAbleToBacktrack() const {
    return !states.empty();
}

size_t WFC::Backtracker::getLastIteration() const {
    return lastIteration;
}

void WFC::Backtracker::setBacktracking(bool backtracking) {
    this->backtracking = backtracking;
}

bool WFC::Backtracker::isBacktracking() const {
    if (options.enabled) {
        return backtracking;
    }
    return false;
}

void WFC::Backtracker::pushBacktrackedState(const State &state) {
    backtrackedStates.push_front(state);
    if (backtrackedStates.size() > options.maxDepth) {
        backtrackedStates.pop_back();
    }
}

void WFC::Backtracker::mergeBacktrackedStates() {
    if (backtrackedStates.empty()) {
        return;
    }
    for (auto &state : backtrackedStates) {
        push(state);
    }
    backtrackedStates.clear();
}

void WFC::Backtracker::setLastIteration(size_t lastIteration) {
    this->lastIteration = lastIteration;
}

void WFC::Backtracker::logStates() const {
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

