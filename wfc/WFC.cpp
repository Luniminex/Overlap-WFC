//
// Created by Jakub on 10.03.2024.
//

#include "WFC.h"


WFC::WFC::WFC(const std::string_view &pathToInputImage, AnalyzerOptions &options, BacktrackerOptions &backtrackerOptions,
         size_t width, size_t height) :
        analyzer(options, pathToInputImage),
        backtracker(backtrackerOptions),
        rng(std::random_device{}()),
        savePaths({
                          "../outputs/patterns/generated-patterns.png",
                          "../outputs/solution.png",
                          "../outputs/failed-solution.png",
                          "../outputs/iterations/",
                          true
                  }),
        status(WFCStatus::PREPARING) {
    outWidth = width;
    outHeight = height;
    state.iteration = 0;
    Util::Logger::log(Util::LogLevel::Info, "WFC initialized and is ready to start");
}

void WFC::WFC::setAnalyzerOptions(const AnalyzerOptions &options) {
    analyzer.setOptions(options);
}

void WFC::WFC::enableBacktracker() {
    backtracker.setEnabled(true);
}

void WFC::WFC::disableBacktracker() {
    backtracker.setEnabled(false);
}

void WFC::WFC::setBacktrackerOptions(const BacktrackerOptions &options) {
    backtracker.setOptions(options);
}

void WFC::WFC::prepareWFC() {
    analyzer.analyze();
    createDirectories();
    //initialize coeff matrix to be outputSize x outputSize x unique patterns count
    logState();
    state.state = std::vector<std::vector<std::vector<bool>>>(
            outHeight, std::vector<std::vector<bool>>(
                    outWidth, std::vector<bool>(
                            analyzer.getPatterns().size(), true)
            ));
    logState();
    //initialize collapsed tiles to be outputSize x outputSize with invalid value
    state.collapsed = std::vector<std::vector<int>>(outHeight, std::vector<int>(outWidth, -1));
    if (savePaths.savePatterns) {
        analyzer.savePatternsPreviewTo(savePaths.generatedPatternsDir);
    }
}

void WFC::WFC::setSavePaths(const WFCSavePaths &paths) {
    savePaths = paths;
}

bool WFC::WFC::startWFC() {
    Util::Timer timer("startWFC");
    status = WFCStatus::RUNNING;
    size_t globalIterations = 0;
    while (status == WFCStatus::RUNNING) {
        Util::Logger::log(Util::LogLevel::Debug, "Iteration: " + std::to_string(state.iteration));

        auto lowestEntropy = Observe();

        if (status == WFCStatus::CONTRADICTION) {
            Util::Logger::log(Util::LogLevel::Important, "Contradiction found");
            //display the last entropy matrix if contradiction was found
            std::vector<std::vector<double>> entropyMatrix(outHeight,
                                                           std::vector<double>(outWidth, 0.0));
            fillEntropyMatrix(entropyMatrix);

            saveOutputImage();
            break;
        }

        if (status == WFCStatus::SOLUTION) {
            Util::Logger::log(Util::LogLevel::Important, "Solution found");
            saveOutputImage();
            break;
        }

        //if backtracking on, skip propagation
        if (lowestEntropy != Util::Point(-1, -1)) {
            propagate(lowestEntropy);
            state.iteration++;
        }

        if (savePaths.savePatterns) {
            displayOutputImage(
                    savePaths.iterationsDir,
                    "g" + std::to_string(globalIterations) + "_l" +
                    std::to_string(state.iteration) +
                    ".png", false);
        }

        globalIterations++;
    }
    Util::Logger::log(Util::LogLevel::Info, "WFC finished in " + std::to_string(state.iteration) + " iterations");
    return status == WFCStatus::SOLUTION;
}

Util::Point WFC::WFC::Observe() {
    static std::chrono::duration<double> observeTime;
    Util::Timer timer("Observe function");

    Util::Logger::log(Util::LogLevel::Debug, "backtrack check");
    if (backtracker.isBacktracking()) {
        if (backtracker.getLastIteration() == state.iteration) {
            backtracker.setBacktracking(false);
            backtracker.mergeBacktrackedStates();
            Util::Logger::log(Util::LogLevel::Debug, "Ended backtracking");
        }
    }

    Util::Point minEntropyPoint{-1, -1};
    //calculate entropy for each cell
    std::vector<std::vector<double>> entropyMatrix(outHeight,
                                                   std::vector<double>(outWidth, 0.0));
    //log entropy matrix
    fillEntropyMatrix(entropyMatrix);
    logEntropyMatrix(entropyMatrix);

    Util::Logger::log(Util::LogLevel::Debug, "checking contradiction");
    if (!checkForContradiction()) {
        if (backtracker.isEnabled() && backtracker.isAbleToBacktrack()) {
            //if just started to backtrack, set this as the last iteration that it should aim for
            if (!backtracker.isBacktracking()) {
                backtracker.setLastIteration(state.iteration + 1);
            }
            Util::Logger::log(Util::LogLevel::Debug, "Drawing from backtracker");
            state = backtracker.draw();
            if (!backtracker.isAbleToBacktrack()) {
                status = WFCStatus::CONTRADICTION;
            }
            backtracker.setBacktracking(true);
            Util::Logger::log(Util::LogLevel::Debug, "Backtracking");
        } else {
            Util::Logger::log(Util::LogLevel::Info, "Contradiction found");
            status = WFCStatus::CONTRADICTION;
        }
        Util::Logger::log(Util::LogLevel::Debug, "was found, returning min point");
        return minEntropyPoint;
    } else {
        Util::Logger::log(Util::LogLevel::Debug, "finding min entropy point");
        std::vector<double> minEntropyProbabilities;
        std::tie(minEntropyPoint, minEntropyProbabilities) = findMinEntropyPoint(entropyMatrix);
        if (status == WFCStatus::SOLUTION) {
            return minEntropyPoint;
        }
        Util::Logger::log(Util::LogLevel::Debug, "found, starting to collapse");
        collapseCell(minEntropyPoint, minEntropyProbabilities);
    }
    return minEntropyPoint;
}

void WFC::WFC::createDirectories() {
    Util::FileUtil::checkDirectory(savePaths.generatedPatternsDir);
    Util::FileUtil::checkDirectory(savePaths.outputImageDir);
    Util::FileUtil::checkDirectory(savePaths.failedOutputImageDir);
    Util::FileUtil::checkDirectory(savePaths.iterationsDir);
    savePaths.iterationsDir = Util::FileUtil::checkAndCreateUniqueDirectory(savePaths.iterationsDir + "iteration") + "/";
}

void WFC::WFC::fillEntropyMatrix(std::vector<std::vector<double>> &entropyMatrix) {
    for (size_t i = 0; i < outHeight; i++) {
        for (size_t j = 0; j < outWidth; j++) {
            //log which for which point its checking
            Util::Logger::log(Util::LogLevel::Debug,
                        "calculating entropy for point: " + std::to_string(j) + " " + std::to_string(i));
            entropyMatrix[i][j] = getShannonEntropy({static_cast<int>(j), static_cast<int>(i)});
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

bool WFC::WFC::checkForContradiction() const {
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

double WFC::WFC::getShannonEntropy(const Util::Point &p) const {
    double sumWeights = 0;
    double sumLogWeights = 0;
    for (size_t option = 0; option < state.state.at(p.y).at(p.x).size(); option++) {
        if (state.state.at(p.y).at(p.x).at(option)) {
            double weight = analyzer.getProbabilities().at(option);
            sumWeights += weight;
            sumLogWeights += weight * std::log(weight);
        }

    }
    if (sumWeights == 0) {
        return 0;
    }
    return std::log(sumWeights) - (sumLogWeights / sumWeights);
}

std::pair<Util::Point, std::vector<double>>
WFC::WFC::findMinEntropyPoint(std::vector<std::vector<double>> &entropyMatrix) {
    double minEntropy = std::numeric_limits<double>::max();
    Util::Point minPoint(-1, -1);

    //find the minimum entropy, if all collapsed, min point is -1, -1
    for (int i = 0; i < entropyMatrix.size(); i++) {
        for (int j = 0; j < entropyMatrix[i].size(); j++) {
            if (state.collapsed[i][j] != -1) {
                continue;
            }
            if (entropyMatrix[i][j] > 0 && entropyMatrix[i][j] < minEntropy) {
                minEntropy = entropyMatrix[i][j];
                minPoint = Util::Point(j, i);
            }
        }
    }

    //if no min entropy point found, solution is found
    if (minPoint.x == -1 && minPoint.y == -1) {
        status = WFCStatus::SOLUTION;
        return {minPoint, {}};
    }

    //finds all possible points with that min entropy
    std::vector<Util::Point> minEntropyPositions;
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
        minEntropyProbabilities[k] = state.state[minPoint.y][minPoint.x][k] ? analyzer.getProbabilities()[k] : 0.0;
    }

    return {minPoint, minEntropyProbabilities};
}

void WFC::WFC::collapseCell(const Util::Point &p, std::vector<double> &probabilities) {
    static std::chrono::duration<double> collapseTime;

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
}

void WFC::WFC::propagate(Util::Point &minEntropyPoint) {
    static std::chrono::duration<double> propagationTime;
    Util::Timer timer("propagate function");
    std::deque<Util::Point> propagationQueue;
    std::set<Util::Point> visited;
    propagationQueue.push_back(minEntropyPoint);
    visited.insert(minEntropyPoint);
    while (!propagationQueue.empty()) {
        Util::Point currentPoint = propagationQueue.front();
        propagationQueue.pop_front();
        for (const auto &offset: analyzer.getOffsets()) {
            //get neighbor pos from current pos and offset
            Util::Point neighborPoint = wrapPoint(currentPoint, offset);

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
    }
}

std::pair<bool, bool>
WFC::WFC::updateCell(const Util::Point &current, const Util::Point &neighbour, const Util::Point &offset) {
    //patterns for current and neighbour cell
    std::vector<bool> &currentPatterns = state.state[current.y][current.x];
    std::vector<bool> &neighbourPatterns = state.state[neighbour.y][neighbour.x];

    std::vector<bool> possiblePatternsInOffset(neighbourPatterns.size(), false);
    for (size_t i = 0; i < currentPatterns.size(); i++) {
        if (!currentPatterns[i]) {
            continue;
        }
        const auto &rules = analyzer.getRules().at(i);
        for (const auto &possiblePattern: rules.at(offset)) {
            possiblePatternsInOffset[possiblePattern] = true;
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

Util::Point WFC::WFC::wrapPoint(const Util::Point &p, const Util::Point &offset) const {
    Util::Point wrappedPoint = p + offset;
    wrappedPoint.x = (wrappedPoint.x + static_cast<int>(outWidth)) % static_cast<int>(outWidth);
    wrappedPoint.y = (wrappedPoint.y + static_cast<int>(outHeight)) % static_cast<int>(outHeight);
    return wrappedPoint;
}

bool WFC::WFC::isPointValid(const Util::Point &p) const {
    return p.x >= 0 && p.y >= 0 && p.x < outWidth && p.y < outHeight;
}

void WFC::WFC::displayOutputImage(const std::string &dir, const std::string &fileName, bool checkForUniqueFilename) const {
    if (state.state.empty()) {
        Util::Logger::log(Util::LogLevel::Error, "State is empty, unable to display image");
        return;
    }

    //i had the height and weight switched for god knows how long and god damn it took me so long to fix this
    cimg_library::CImg<unsigned char> res(outWidth, outHeight, 1, 3, 0);
    for (int y = 0; y < outHeight; y++) {
        for (int x = 0; x < outWidth; x++) {
            std::vector<cimg_library::CImg<unsigned char>> validPatterns;
            for (int pattern = 0; pattern < analyzer.getPatterns().size(); pattern++) {
                if (state.state.at(y).at(x).at(pattern)) {
                    validPatterns.emplace_back(analyzer.getPatterns().at(pattern));
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

    //directory + file name
    std::string filePath = dir + fileName;
    if (checkForUniqueFilename) {
        filePath = Util::FileUtil::getUniqueFileName(filePath);
    }

    res.save_png(filePath.c_str());
}

void WFC::WFC::logEntropyMatrix(const std::vector<std::vector<double>> &entropyMatrix) const {
    std::stringstream ss;
    for (const auto &i: entropyMatrix) {
        ss << "[ ";
        for (double j: i) {
            ss << std::fixed << std::setprecision(2) << j << " ";
        }
        ss << "]\n";
    }
    Util::Logger::log(Util::LogLevel::Debug, "Entropy matrix: \n" + ss.str());
}

void WFC::WFC::logState() {
    std::stringstream ss;
    for (auto &i: state.state) {
        ss << "[ ";
        for (const auto &j: i) {
            ss << " ";
            ss << j.size();
        }
        ss << " ] ";
        ss << "\n";
    }
    Util::Logger::log(Util::LogLevel::Debug, "State: \n" + ss.str());
}

void WFC::WFC::saveOutputImage() {
    //i had the height and weight switched for god knows how long and god damn it took me so long to fix this
    outputImage = cimg_library::CImg<unsigned char>(outWidth, outHeight, 1, 3, 0);
    for (int y = 0; y < outHeight; y++) {
        for (int x = 0; x < outWidth; x++) {
            std::vector<cimg_library::CImg<unsigned char>> validPatterns;
            for (int pattern = 0; pattern < analyzer.getPatterns().size(); pattern++) {
                if (state.state.at(y).at(x).at(pattern)) {
                    validPatterns.emplace_back(analyzer.getPatterns().at(pattern));
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

                outputImage(x, y, 0) = meanPattern(0, 0, 0);
                outputImage(x, y, 1) = meanPattern(0, 0, 1);
                outputImage(x, y, 2) = meanPattern(0, 0, 2);
            }
        }
    }
}

void WFC::WFC::saveOutput() const {
    std::string fileName;
    std::string dir;
    if(status == WFCStatus::SOLUTION) {
        fileName = "solution.png";
        dir = savePaths.outputImageDir;
    }
    else if(status == WFCStatus::CONTRADICTION) {
        fileName = "contradiction.png";
        dir = savePaths.failedOutputImageDir;
    }
    else {
        Util::Logger::log(Util::LogLevel::Error, "Cannot save output, WFC did not finish");
        return;
    }

    std::string filePath = dir + fileName;
    filePath = Util::FileUtil::getUniqueFileName(filePath);
    outputImage.save_png(filePath.c_str());
}
