//
// Created by Jakub on 24.04.2024.
//

#ifndef WFC_BACKTRACKER_H
#define WFC_BACKTRACKER_H

#include <vector>
#include <stack>
#include <deque>
#include <cstddef>
#include <iostream>

#include "../utility/Logger.h"

namespace WFC {

    using imageState = std::vector<std::vector<std::vector<bool>>>;
    using collapsedState = std::vector<std::vector<int>>;

    struct State {
        imageState state;
        collapsedState collapsed;
        size_t iteration;
    };

    struct BacktrackerOptions {
        unsigned int maxDepth;
        //max iterations on the same level
        unsigned int maxIterations;
        bool enabled;
    };

    class Backtracker {
    public:
        Backtracker();

        explicit Backtracker(BacktrackerOptions options);

        void push(const State &state);

        void pushBacktrackedState(const State &state);

        void mergeBacktrackedStates();

        State draw();

        [[nodiscard]] bool isEnabled() const;

        void setEnabled(bool enabled);

        void setOptions(const BacktrackerOptions &options);

        void setBacktracking(bool backtracking);

        [[nodiscard]] bool isBacktracking() const;

        void setLastIteration(size_t lastIteration);

        [[nodiscard]] bool isAbleToBacktrack() const;

        [[nodiscard]] size_t getLastIteration() const;

        void logStates() const;

    private:
        std::deque<std::pair<State, size_t>> states;
        //if backtracking, push states to this, then merge
        std::deque<State> backtrackedStates;
        BacktrackerOptions options;
        size_t lastIteration;
        bool backtracking;
    };

}
#endif //WFC_BACKTRACKER_H