//
// Created by Jakub on 10.03.2024.
//

#include "WFC.h"


WFC::WFC(const std::string_view &pathToInputImage, AnalyzerOptions &options, BacktrackerOptions &backtrackerOptions,
         size_t width, size_t height) :
        analyzer(options, pathToInputImage),
        backtracker(backtrackerOptions),
        rng(std::random_device{}()),
        status(WFCStatus::PREPARING) {
    this->outWidth = width;
    this->outHeight = height;
    state.iteration = 0;
    Logger::log(LogLevel::Info, "WFC initialized and is ready to start");
}

void WFC::setAnalyzerOptions(const AnalyzerOptions &options) {
    analyzer.setOptions(options);
}

void WFC::enableBacktracker() {
    backtracker.setEnabled(true);
}

void WFC::disableBacktracker() {
    backtracker.setEnabled(false);
}

void WFC::setBacktrackerOptions(const BacktrackerOptions &options) {
    backtracker.setOptions(options);
}

bool WFC::prepareWFC(bool savePatterns, const std::string &path) {
    bool success = analyzer.analyze();

    //initialize coeff matrix to be outputSize x outputSize x unique patterns count
    const auto &patterns = analyzer.getPatterns();
    logState();
    state.state = std::vector<std::vector<std::vector<bool>>>(
            outHeight, std::vector<std::vector<bool>>(
                    outWidth, std::vector<bool>(
                            patterns.size(), true)
            ));
    logState();
    //initialize collapsed tiles to be outputSize x outputSize with invalid value
    state.collapsed = std::vector<std::vector<int>>(outHeight, std::vector<int>(outWidth, -1));
    if (savePatterns) {
        analyzer.savePatternsPreviewTo(path);
    }
    return success;
}

bool WFC::startWFC(bool saveIterations, const std::string &dir) {
    Timer timer("startWFC");
    status = WFCStatus::RUNNING;
    size_t globalIterations = 0;
    while (status == WFCStatus::RUNNING) {
        Logger::log(LogLevel::Debug, "Iteration: " + std::to_string(state.iteration));

        auto lowestEntropy = Observe();

        if (status == WFCStatus::CONTRADICTION) {
            Logger::log(LogLevel::Info, "Contradiction found");
            std::vector<std::vector<double>> entropyMatrix(outHeight,
                                                           std::vector<double>(outWidth, 0.0));
            fillEntropyMatrix(entropyMatrix);
            displayOutputImage("../outputs/failed-solution.png");
            break;
        }

        if (status == WFCStatus::SOLUTION) {
            Logger::log(LogLevel::Info, "Solution found");
            displayOutputImage("../outputs/solution.png");
            break;
        }

        //if backtracking on, skip propagation
        if (lowestEntropy != Point(-1, -1)) {
            propagate(lowestEntropy);
            state.iteration++;
        }

        if (saveIterations) {
            displayOutputImage(
                    dir + "g" + std::to_string(globalIterations) + "_l" +
                    std::to_string(state.iteration) +
                    ".png");
        }

        globalIterations++;
    }
    Logger::log(LogLevel::Info, "WFC finished in " + std::to_string(state.iteration) + " iterations");
    return status == WFCStatus::SOLUTION;
}

Point WFC::Observe() {
    static std::chrono::duration<double> observeTime;
    Timer timer("Observe function");

    Logger::log(LogLevel::Debug, "backtrack check");
    if (backtracker.isBacktracking()) {
        if (backtracker.getLastIteration() == state.iteration) {
            backtracker.setBacktracking(false);
            backtracker.mergeBacktrackedStates();
            Logger::log(LogLevel::Debug, "Ended backtracking");
        }
    }

    Point minEntropyPoint{-1, -1};
    //calculate entropy for each cell
    std::vector<std::vector<double>> entropyMatrix(outHeight,
                                                   std::vector<double>(outWidth, 0.0));
    Logger::log(LogLevel::Debug, "filling entropy");
    //log entropy matrix
    logEntropyMatrix(entropyMatrix);
    fillEntropyMatrix(entropyMatrix);
    Logger::log(LogLevel::Debug, "filled entropy");

    Logger::log(LogLevel::Debug, "checking contradiction");
    if (!checkForContradiction()) {
        if (backtracker.isEnabled() && backtracker.isAbleToBacktrack()) {
            //if just started to backtrack, set this as the last iteration that it should aim for
            if (!backtracker.isBacktracking()) {
                backtracker.setLastIteration(state.iteration + 1);
            }
            Logger::log(LogLevel::Debug, "Drawing from backtracker");
            state = backtracker.draw();
            if (!backtracker.isAbleToBacktrack()) {
                status = WFCStatus::CONTRADICTION;
            }
            backtracker.setBacktracking(true);
            Logger::log(LogLevel::Debug, "Backtracking");
        } else {
            Logger::log(LogLevel::Info, "Contradiction found");
            status = WFCStatus::CONTRADICTION;
        }
        Logger::log(LogLevel::Debug, "was found, returning min point");
        return minEntropyPoint;
    } else {
        Logger::log(LogLevel::Debug, "finding min entropy point");
        std::vector<double> minEntropyProbabilities;
        std::tie(minEntropyPoint, minEntropyProbabilities) = findMinEntropyPoint(entropyMatrix);
        if (status == WFCStatus::SOLUTION) {
            return minEntropyPoint;
        }
        Logger::log(LogLevel::Debug, "found, starting to collapse");
        collapseCell(minEntropyPoint, minEntropyProbabilities);
        observeTime += timer.getCurrent();
        Logger::log(LogLevel::Debug, "Total observe time:" + std::to_string(observeTime.count()) + "s");
    }
    return minEntropyPoint;
}

void WFC::fillEntropyMatrix(std::vector<std::vector<double>> &entropyMatrix) {
    for (size_t i = 0; i < outHeight; i++) {
        for (size_t j = 0; j < outWidth; j++) {
            //log which for which point its checking
            Logger::log(LogLevel::Debug,
                        "calculating entropy for point: " + std::to_string(j) + " " + std::to_string(i));
            try {
                entropyMatrix[i][j] = getShannonEntropy({static_cast<int>(j), static_cast<int>(i)});
            } catch (const std::out_of_range &e) {
                Logger::log(LogLevel::Debug, "Out of range exception caught: " + std::string(e.what()));
                //log point
                Logger::log(LogLevel::Debug, "Point: " + std::to_string(j) + " " + std::to_string(i));
            }
            /*for (size_t k = 0; k < coeffMatrix[i][j].size(); ++k) {
                if (coeffMatrix[i][j][k]) {
                    entropyMatrix[i][j] += probabilities[k];
                }
            }*/
        }
    }
    logEntropyMatrix(entropyMatrix);
    // Set entropy to 0 for all collapsed cells
    for (size_t i = 0; i < outHeight; i++) {
        for (size_t j = 0; j < outWidth; j++) {
            if (state.collapsed[i][j] != -1) {
                entropyMatrix[i][j] = 0;
            }
        }
    }

    logEntropyMatrix(entropyMatrix);
}

bool WFC::checkForContradiction() const {
    //if any cell has no possible patterns then there is a contradiction
    for (size_t i = 0; i < outHeight; i++) {
        for (size_t j = 0; j < outWidth; j++) {
            if (std::count(state.state[i][j].begin(), state.state[i][j].end(), true) == 0) {
                return false;
            }
        }
    }
    return true;
}

double WFC::getShannonEntropy(const Point &p) const {
    double sumWeights = 0;
    double sumLogWeights = 0;
    try {
        for (size_t option = 0; option < state.state.at(p.y).at(p.x).size(); option++) {
            try {
                if (state.state.at(p.y).at(p.x).at(option)) {
                    double weight = analyzer.getProbs().at(option);
                    sumWeights += weight;
                    sumLogWeights += weight * std::log(weight);
                }
            }
            catch (const std::out_of_range &e) {
                Logger::log(LogLevel::Debug, "Out of range exception caught: " + std::string(e.what()));
            }
        }
    }
    catch (const std::out_of_range &e) {
        Logger::log(LogLevel::Debug, "Out of range exception caught: " + std::string(e.what()));
    }
    if (sumWeights == 0) {
        return 0;
    }
    return std::log(sumWeights) - (sumLogWeights / sumWeights);
}

std::pair<Point, std::vector<double>>
WFC::findMinEntropyPoint(std::vector<std::vector<double>> &entropyMatrix) {
    double minEntropy = std::numeric_limits<double>::max();
    Point minPoint(-1, -1);

    //find the minimum entropy, if all collapsed, min point is -1, -1
    for (int i = 0; i < entropyMatrix.size(); i++) {
        for (int j = 0; j < entropyMatrix[i].size(); j++) {
            if (state.collapsed[i][j] != -1) {
                continue;
            }
            if (entropyMatrix[i][j] > 0 && entropyMatrix[i][j] < minEntropy) {
                minEntropy = entropyMatrix[i][j];
                minPoint = Point(j, i);
            }
        }
    }

    //if no min entropy point found, solution is found
    if (minPoint.x == -1 && minPoint.y == -1) {
        status = WFCStatus::SOLUTION;
        return {minPoint, {}};
    }

    //finds all possible points with that min entropy
    std::vector<Point> minEntropyPositions;
    for (int i = 0; i < entropyMatrix.size(); i++) {
        for (int j = 0; j < entropyMatrix[i].size(); j++) {
            if (state.collapsed[i][j] != -1) {
                continue;
            }
            if (entropyMatrix[i][j] == minEntropy) {
                minEntropyPositions.emplace_back(j, i);
            }
        }
    }

    //picks random point from all possible points with min entropy
    if (minEntropyPositions.size() > 1) {
        std::uniform_int_distribution<size_t> dist(0, minEntropyPositions.size() - 1);
        minPoint = minEntropyPositions[dist(rng)];
    }

    //get possible probabilities for that point
    std::vector<double> minEntropyProbabilities(state.state[minPoint.y][minPoint.x].size());
    for (size_t k = 0; k < state.state[minPoint.y][minPoint.x].size(); k++) {
        //if that pattern is possible, get its probability, else set it to 0
        minEntropyProbabilities[k] = state.state[minPoint.y][minPoint.x][k] ? analyzer.getProbs()[k] : 0.0;
    }

    return {minPoint, minEntropyProbabilities};
}

void WFC::collapseCell(const Point &p, std::vector<double> &probabilities) {
    static std::chrono::duration<double> collapseTime;
    Timer timer("collapse function");

    //if is backtracking enabled and is currently not in backtracking, remember the state
    if (backtracker.isEnabled()) {
        if (backtracker.isBacktracking()) {
            backtracker.pushBacktrackedState(state);
        } else {
            backtracker.push(state);
        }
    }

    //choose a random possible option from probabilities vector and mark that chosen index in collapsedTiles
    std::discrete_distribution<size_t> dist(probabilities.begin(), probabilities.end());
    size_t chosenPattern = dist(rng);
    state.collapsed[p.y][p.x] = static_cast<int>(chosenPattern);

    for (size_t i = 0; i < state.state[p.y][p.x].size(); i++) {
        state.state[p.y][p.x][i] = (i == chosenPattern);
    }
    collapseTime += timer.getCurrent();
    Logger::log(LogLevel::Debug, "Total collapse time:" + std::to_string(collapseTime.count()) + "s");

}

void
WFC::propagate(Point &minEntropyPoint) {
    static std::chrono::duration<double> propagationTime;
    Timer timer("propagate function");
    std::deque<Point> propagationQueue;
    std::set<Point> visited;
    propagationQueue.push_back(minEntropyPoint);
    visited.insert(minEntropyPoint);
    size_t propagations_count = 0;
    while (!propagationQueue.empty()) {
        Point currentPoint = propagationQueue.front();
        propagationQueue.pop_front();
        for (const auto &offset: analyzer.getOffsets()) {
            //get neighbor pos from current pos and offset
            Point neighborPoint = wrapPoint(currentPoint, offset);

            //if neighbor is not valid or is collapsed already, skip it
            if (!isPointValid(neighborPoint) || state.collapsed[neighborPoint.y][neighborPoint.x] != -1) {
                continue;
            }

            //update neighbor cell
            bool updated, collapsed;
            std::tie(updated, collapsed) = updateCell(currentPoint, neighborPoint, offset);

            //if updated, add to propagation queue and not in queue already
            if (updated) {
                if (visited.find(neighborPoint) == visited.end()) {
                    propagationQueue.push_back(neighborPoint);
                    visited.insert(neighborPoint);
                }
            }
        }
        propagations_count++;
    }
    propagationTime += timer.getCurrent();
    Logger::log(LogLevel::Debug, "Total propagation time:" + std::to_string(propagationTime.count()) + "s");
}

std::pair<bool, bool> WFC::updateCell(const Point &current, const Point &neighbour, const Point &offset) {
    //patterns for current and neighbour cell
    std::vector<bool> &currentPatterns = state.state[current.y][current.x];
    std::vector<bool> &neighbourPatterns = state.state[neighbour.y][neighbour.x];

    std::vector<bool> possiblePatternsInOffset(neighbourPatterns.size(), false);
    for (size_t i = 0; i < currentPatterns.size(); i++) {
        if (!currentPatterns[i]) {
            continue;
        }
        const auto &rules = analyzer.getRules().at(i);
        try {
            for (const auto &possiblePattern: rules.at(offset)) {
                possiblePatternsInOffset[possiblePattern] = true;
            }
        } catch (const std::out_of_range &e) {
            Logger::log(LogLevel::Error, "Out of range exception caught: " + std::string(e.what()));
        }
    }


    //multiply the target cell by possible patterns form original cell
    std::vector<bool> updatedCell(state.state[current.y][current.x].size());
    for (size_t i = 0; i < updatedCell.size(); i++) {
        updatedCell[i] = possiblePatternsInOffset[i] && neighbourPatterns[i];
    }

    //check if the cell was updated
    bool isUpdated = (updatedCell != state.state[neighbour.y][neighbour.x]);
    bool isCollapsed = std::count(updatedCell.begin(), updatedCell.end(), true) == 1;

    //set the cell as collapsed
    if (isCollapsed) {
        //find the index of the collapsed pattern
        size_t collapsedPatternIndex = std::find(updatedCell.begin(), updatedCell.end(), true) - updatedCell.begin();

        //set the collapsed pattern in the collapsed tiles matrix
        state.collapsed[neighbour.y][neighbour.x] = static_cast<int>(collapsedPatternIndex);
    }

    //update the cell in the coefficient matrix
    state.state[neighbour.y][neighbour.x] = updatedCell;

    return {isUpdated, isCollapsed};
}

Point WFC::wrapPoint(const Point &p, const Point &offset) const {
    Point wrappedPoint = p + offset;
    wrappedPoint.x = (wrappedPoint.x + outWidth) % outWidth;
    wrappedPoint.y = (wrappedPoint.y + outHeight) % outHeight;
    return wrappedPoint;
}

bool WFC::isPointValid(const Point &p) const {
    return p.x >= 0 && p.y >= 0 && p.x < outWidth && p.y < outHeight;
}

void WFC::displayOutputImage(const std::string &path) const {
    if (state.state.empty()) {
        Logger::log(LogLevel::Error, "State is empty, unable to display image");
        return;
    }

    cimg_library::CImg<unsigned char> res(outWidth, outHeight, 1, 3, 0);
    for (int y = 0; y < outHeight; y++) {
        for (int x = 0; x < outWidth; x++) {
            std::vector<cimg_library::CImg<unsigned char>> validPatterns;
            for (int pattern = 0; pattern < analyzer.getPatterns().size(); pattern++) {
                if (state.state.at(y).at(x).at(pattern)) {
                    auto ptrn = analyzer.getPatterns().at(pattern);
                    validPatterns.emplace_back(ptrn);
                }
            }
            if (!validPatterns.empty()) {
                cimg_library::CImg<unsigned char> meanPattern = validPatterns[0];
                if (validPatterns.size() > 1) {
                    for (size_t i = 1; i < validPatterns.size(); i++) {
                        meanPattern += validPatterns[i];
                    }
                }
                meanPattern /= validPatterns.size();

                res(x, y, 0) = meanPattern(0, 0, 0);
                res(x, y, 1) = meanPattern(0, 0, 1);
                res(x, y, 2) = meanPattern(0, 0, 2);
            }
        }
    }
    res.resize(static_cast<int>(outWidth), static_cast<int>(outHeight), 1, 3, 1);
    res.save_png(path.c_str());
}

void WFC::logEntropyMatrix(const std::vector<std::vector<double>> &entropyMatrix) const {
    std::stringstream ss;
    for (const auto &i: entropyMatrix) {
        ss << "[ ";
        for (double j: i) {
            ss << std::fixed << std::setprecision(2) << j << " ";
        }
        ss << "]\n";
    }
    Logger::log(LogLevel::Debug, "Entropy matrix: \n" + ss.str());
}

void WFC::logState() {
    std::stringstream ss;
    for (size_t i = 0; i < state.state.size(); i++) {
        ss << "[ ";
        for (size_t j = 0; j < state.state[i].size(); j++) {
            ss << " ";
            ss << state.state[i][j].size();
        }
        ss << " ] ";
        ss << "\n";
    }
    Logger::log(LogLevel::Debug, "State: \n" + ss.str());
}