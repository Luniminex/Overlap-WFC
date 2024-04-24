//
// Created by Jakub on 10.03.2024.
//

#include "WFC.h"

WFC::WFC(const std::string_view &pathToInputImage, AnalyzerOptions &options) : analyzer(
        options, pathToInputImage), rng(std::random_device{}()), status(WFCStatus::PREPARING) {
    Logger::log(LogLevel::Info, "WFC initialized and is ready to start");
}

bool WFC::prepareWFC(bool savePatterns, const std::string &path) {
    bool success = analyzer.analyze();

    //initialize coeff matrix to be outputSize x outputSize x unique patterns count
    size_t outputSize = analyzer.getOptions().outputSize;
    const auto &patterns = analyzer.getPatterns();
    coeffMatrix = std::vector<std::vector<std::vector<bool>>>(
            outputSize, std::vector<std::vector<bool>>(
                    outputSize, std::vector<bool>(
                            patterns.size(), True)
            ));
    //initialize collapsed tiles to be outputSize x outputSize with invalid value
    collapsedTiles = std::vector<std::vector<int>>(outputSize, std::vector<int>(outputSize, -1));
    if (savePatterns) {
        analyzer.savePatternsPreviewTo(path);
    }
    startWFC();
    return success;
}


void WFC::startWFC() {
    Timer timer("startWFC");
    status = WFCStatus::RUNNING;
    size_t iterations = 0;
    while (status == WFCStatus::RUNNING) {
        Logger::log(LogLevel::Debug, "Iteration: " + std::to_string(iterations));
        LogCoeffMatrix();
        //logEntropyQueue();

        //find cell with lowest entropy
        auto lowestEntropy = Observe();

        if (status == WFCStatus::CONTRADICTION) {
            Logger::log(LogLevel::Info, "Contradiction found");

            std::vector<std::vector<double>> entropyMatrix(analyzer.getOutputSize(),
                                                           std::vector<double>(analyzer.getOutputSize(), 0.0));
            fillEntropyMatrix(entropyMatrix);

            size_t size = analyzer.getOutputSize() * analyzer.getOptions().scale;
            displayOutputImage("../outputs/failed-solution.png", size, size);
            break;
        }
        if (status == WFCStatus::SOLUTION) {
            Logger::log(LogLevel::Info, "Solution found");
            size_t size = analyzer.getOutputSize() * analyzer.getOptions().scale;
            displayOutputImage("../outputs/solution.png", size, size);
            break;
        }

        propagate(lowestEntropy);
        iterations++;
    }
    Logger::log(LogLevel::Info, "WFC finished in " + std::to_string(iterations) + " iterations");
}


Point WFC::Observe() {
    static std::chrono::duration<double> observeTime;
    Timer timer("Observe function");
    //calculate entropy for each cell
    std::vector<std::vector<double>> entropyMatrix(analyzer.getOutputSize(),
                                                   std::vector<double>(analyzer.getOutputSize(), 0.0));
    fillEntropyMatrix(entropyMatrix);

    if (!checkForContradiction()) {
        status = WFCStatus::CONTRADICTION;
        return {-1, -1};
    }

    Point minEntropyPoint{-1, -1};
    std::vector<double> minEntropyProbabilities;
    std::tie(minEntropyPoint, minEntropyProbabilities) = findMinEntropyPoint(entropyMatrix);
    if (status == WFCStatus::SOLUTION) {
        return minEntropyPoint;
    }
    collapseCell(minEntropyPoint, minEntropyProbabilities);

    observeTime += timer.getCurrent();
    Logger::log(LogLevel::Debug, "Total observe time:" + std::to_string(observeTime.count()) + "s");

    return minEntropyPoint;
}

void WFC::fillEntropyMatrix(std::vector<std::vector<double>> &entropyMatrix) {
    size_t outputSize = analyzer.getOutputSize();
    for (size_t i = 0; i < outputSize; ++i) {
        for (size_t j = 0; j < outputSize; ++j) {
            entropyMatrix[i][j] = getShannonEntropy({static_cast<int>(j), static_cast<int>(i)});
            /*for (size_t k = 0; k < coeffMatrix[i][j].size(); ++k) {
                if (coeffMatrix[i][j][k]) {
                    entropyMatrix[i][j] += probabilities[k];
                }
            }*/
        }
    }
    // Set entropy to 0 for all collapsed cells
    for (size_t i = 0; i < outputSize; ++i) {
        for (size_t j = 0; j < outputSize; ++j) {
            if (collapsedTiles[i][j] != -1) {
                entropyMatrix[i][j] = 0;
            }
        }
    }

    logEntropyMatrix(entropyMatrix);
}

bool WFC::checkForContradiction() const {
    size_t outputSize = analyzer.getOutputSize();
    //if any cell has no possible patterns then there is a contradiction
    for (size_t i = 0; i < outputSize; i++) {
        for (size_t j = 0; j < outputSize; j++) {
            if (std::count(coeffMatrix[i][j].begin(), coeffMatrix[i][j].end(), true) == 0) {
                return false;
            }
        }
    }
    return true;
}

size_t WFC::getEntropy(Point p) const {
    return std::count(coeffMatrix[p.y][p.x].begin(), coeffMatrix[p.y][p.x].end(), true);
}

double WFC::getShannonEntropy(const Point &p) const {
    double sumWeights = 0;
    double sumLogWeights = 0;
    for (size_t option = 0; option < coeffMatrix[p.y][p.x].size(); option++) {
        if (coeffMatrix[p.y][p.x].at(option)) {
            double weight = analyzer.getProbs().at(option);
            sumWeights += weight;
            sumLogWeights += weight * std::log(weight);
        }
    }
    if(sumWeights == 0) {
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
            if (collapsedTiles[i][j] != -1) {
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
        LogCoeffMatrix();
        status = WFCStatus::SOLUTION;
        return {minPoint, {}};
    }

    //finds all possible points with that min entropy
    std::vector<Point> minEntropyPositions;
    for (int i = 0; i < entropyMatrix.size(); i++) {
        for (int j = 0; j < entropyMatrix[i].size(); j++) {
            if (collapsedTiles[i][j] != -1) {
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
    std::vector<double> minEntropyProbabilities(coeffMatrix[minPoint.y][minPoint.x].size());
    for (size_t k = 0; k < coeffMatrix[minPoint.y][minPoint.x].size(); ++k) {
        //if that pattern is possible, get its probability, else set it to 0
        minEntropyProbabilities[k] = coeffMatrix[minPoint.y][minPoint.x][k] ? analyzer.getProbs()[k] : 0.0;
    }

    return {minPoint, minEntropyProbabilities};
}

void WFC::collapseCell(const Point &p, std::vector<double> &probabilities) {
    static std::chrono::duration<double> collapseTime;
    Timer timer("collapse function");
    //choose a random possible option from probabilities vector and mark that chosen index in collapsedTiles
    std::discrete_distribution<size_t> dist(probabilities.begin(), probabilities.end());
    size_t chosenPattern = dist(rng);
    collapsedTiles[p.y][p.x] = chosenPattern;

    for (size_t i = 0; i < coeffMatrix[p.y][p.x].size(); ++i) {
        coeffMatrix[p.y][p.x][i] = (i == chosenPattern);
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
        LogPossibleRulesForPoint(currentPoint);
        for (const auto &offset: analyzer.getOffsets()) {
            //get neighbor pos from current pos and offset
            Point neighborPoint = wrapPoint(currentPoint, offset);

            //if neighbor is not valid or is collapsed already, skip it
            if (!isPointValid(neighborPoint) || collapsedTiles[neighborPoint.y][neighborPoint.x] != -1) {
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
    std::vector<bool> &currentPatterns = coeffMatrix[current.y][current.x];
    std::vector<bool> &neighbourPatterns = coeffMatrix[neighbour.y][neighbour.x];

    std::vector<bool> possiblePatternsInOffset(neighbourPatterns.size(), false);
    size_t i = 0;
    for (i = 0; i < currentPatterns.size(); i++) {
        if (!currentPatterns[i]) {
            continue;
        }
        const auto &rules = analyzer.getRules().at(i);
        try{
        for (const auto &possiblePattern: rules.at(offset)) {
            possiblePatternsInOffset[possiblePattern] = true;
        }
        } catch (const std::out_of_range& e) {
            Logger::log(LogLevel::Error, "Out of range exception caught: " + std::string(e.what()));
        }
    }


    //multiply the target cell by possible patterns form original cell
    std::vector<bool> updatedCell(coeffMatrix[current.y][current.x].size());
    for (size_t i = 0; i < updatedCell.size(); i++) {
        updatedCell[i] = possiblePatternsInOffset[i] && neighbourPatterns[i];
    }

    //check if the cell was updated
    bool isUpdated = (updatedCell != coeffMatrix[neighbour.y][neighbour.x]);
    bool isCollapsed = std::count(updatedCell.begin(), updatedCell.end(), true) == 1;

    //set the cell as collapsed
    if (isCollapsed) {
        //find the index of the collapsed pattern
        size_t collapsedPatternIndex = std::find(updatedCell.begin(), updatedCell.end(), true) - updatedCell.begin();

        //set the collapsed pattern in the collapsed tiles matrix
        collapsedTiles[neighbour.y][neighbour.x] = collapsedPatternIndex;
    }

    //update the cell in the coefficient matrix
    coeffMatrix[neighbour.y][neighbour.x] = updatedCell;

    return {isUpdated, isCollapsed};
}

Point WFC::wrapPoint(const Point &p, const Point &offset) const {
    Point wrappedPoint = p + offset;
    auto outputSize = analyzer.getOutputSize();
    if (wrappedPoint.x < 0) {
        wrappedPoint.x = outputSize - 1;
    }
    if (wrappedPoint.y < 0) {
        wrappedPoint.y = outputSize - 1;
    }
    if (wrappedPoint.x >= outputSize) {
        wrappedPoint.x = 0;
    }
    if (wrappedPoint.y >= outputSize) {
        wrappedPoint.y = 0;
    }
    return wrappedPoint;
}

bool WFC::isPointValid(const Point &p) const {
    auto outputSize = analyzer.getOutputSize();
    return p.x >= 0 && p.y >= 0 && p.x < outputSize && p.y < outputSize;
}

void WFC::logRules() const {
    auto &rules = analyzer.getRules();
    auto &offsets = analyzer.getOffsets();
    auto &patterns = analyzer.getPatterns();
    // Create a vector to store the total number of rules for each pattern
    std::vector<size_t> totalRulesPerPattern(rules.size(), 0);
    // Iterate over all patterns
    for (size_t i = 0; i < rules.size(); i++) {
        // Iterate over all offsets for the current pattern
        for (const auto &rule: rules[i]) {
            // Increment the counter for the current pattern by the number of rules at the current offset
            totalRulesPerPattern[i] += rule.second.size();
        }
    }

    // Log the total number of rules for each pattern
    for (size_t i = 0; i < totalRulesPerPattern.size(); i++) {
        Logger::log(LogLevel::Debug, "Total number of rules for pattern " + std::to_string(i) + " is " +
                                     std::to_string(totalRulesPerPattern[i]));
    }

    for (size_t patternIndex = 0; patternIndex < patterns.size(); ++patternIndex) {
        Logger::log(LogLevel::Debug, "Pattern number " + std::to_string(patternIndex) + ":");
        for (const auto &offset: offsets) {
            std::string possiblePatternsStr;
            auto it = rules[patternIndex].find(offset);
            if (it == rules[patternIndex].end()) {
                possiblePatternsStr = "empty";
            } else {
                for (const auto &pattern: it->second) {
                    possiblePatternsStr += std::to_string(pattern) + ", ";
                }
            }
            Logger::log(LogLevel::Debug,
                        "key: (" + std::to_string(offset.x) + ", " + std::to_string(offset.y) + "), values: {" +
                        possiblePatternsStr + "}");
        }
    }
}

void WFC::LogPossibleRulesForPoint(Point point) const {
    //get all possible patterns for that point
    std::string possiblePetternsStr = "";
    for (size_t i = 0; i < coeffMatrix[point.y][point.x].size(); i++) {
        if (coeffMatrix[point.y][point.x][i]) {
            possiblePetternsStr += std::to_string(i) + " ";
        }
    }

    // Log the chosen pattern
    /*Logger::log(LogLevel::Debug,
                "Possible patterns at point (" + std::to_string(point.x) + ", " + std::to_string(point.y) + ") is [ " +
                possiblePetternsStr + "] ");*/

    //iterate over all offsets
    /* for (const auto &offset: offsets) {
         const auto &possiblePatterns = rules.at(chosenPattern).at(offset);

         std::string possiblePatternsStr;
         for (const auto &pattern: possiblePatterns) {
             possiblePatternsStr += std::to_string(pattern) + " ";
         }


     Logger::log(LogLevel::Debug,
                 "Possible patterns at offset (" + std::to_string(offset.x) + ", " + std::to_string(offset.y) +
                 ") are " + possiblePatternsStr);
     }*/
}

void WFC::LogCoeffMatrix() const {
    /*   std::string coeffMatrixStr;
       for (size_t i = 0; i < coeffMatrix.size(); i++) {
           coeffMatrixStr += "[ ";
           for (size_t j = 0; j < coeffMatrix[i].size(); j++) {
               coeffMatrixStr += "[ ";
               for (size_t k = 0; k < coeffMatrix[i][j].size(); k++) {
                   if (coeffMatrix[i][j][k])
                       coeffMatrixStr += std::to_string(k) + " ";
                   else
                       coeffMatrixStr += "X ";
               }
               coeffMatrixStr += "] ";
           }
           coeffMatrixStr += "]\n";
       }
       Logger::log(LogLevel::Debug, "Coeff matrix: \n" + coeffMatrixStr);
   */


    //save the image to a file
    static int index = 0;
    std::string path = "../outputs/iterations/iteration" + std::to_string(index) + ".png";
   // displayOutputImage(path, 128, 128);
    index++;
}

void WFC::displayOutputImage(const std::string &path, int width, int height) const {
    cimg_library::CImg<unsigned char> res(coeffMatrix.size(), coeffMatrix[0].size(), 1, 3, 0);
    auto &patterns = analyzer.getPatterns();

    for (int row = 0; row < coeffMatrix.size(); ++row) {
        for (int col = 0; col < coeffMatrix[row].size(); ++col) {
            std::vector<cimg_library::CImg<unsigned char>> validPatterns;
            for (int pattern = 0; pattern < patterns.size(); ++pattern) {
                if (coeffMatrix[row][col][pattern]) {
                    validPatterns.emplace_back(patterns[pattern]);
                }
            }
            if (!validPatterns.empty()) {
                cimg_library::CImg<unsigned char> meanPattern = validPatterns[0];
                for (size_t i = 1; i < validPatterns.size(); ++i) {
                    meanPattern += validPatterns[i];
                }
                meanPattern /= validPatterns.size();

                res(col, row, 0) = meanPattern(0, 0, 0);
                res(col, row, 1) = meanPattern(0, 0, 1);
                res(col, row, 2) = meanPattern(0, 0, 2);
            }
        }
    }

    res.resize(width, height, 1, 3, 1);
    res.save_png(path.c_str());
}

void WFC::logEntropyMatrix(const std::vector<std::vector<double>> &entropyMatrix) const {
    std::stringstream ss;
    for (size_t i = 0; i < entropyMatrix.size(); i++) {
        ss << "[ ";
        for (size_t j = 0; j < entropyMatrix[i].size(); j++) {
            ss << std::fixed << std::setprecision(2) << entropyMatrix[i][j] << " ";
        }
        ss << "]\n";
    }
    Logger::log(LogLevel::Debug, "Entropy matrix: \n" + ss.str());
}
