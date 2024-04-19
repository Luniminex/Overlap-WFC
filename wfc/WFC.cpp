//
// Created by Jakub on 10.03.2024.
//

#include "WFC.h"

WFC::WFC(const std::string_view &inputImage, const size_t &outputSize, const AnalyzerOptions &options) : options(
        options), rng(std::random_device{}()), status(WFCStatus::PREPARING) {

    this->inputImage = cimg::CImg<unsigned char>(inputImage.data());
    if (options.patternSize > outputSize) {
        Logger::log(LogLevel::Warning, "Pattern size cannot be greater than output size");
    } else {
        this->outputSize = outputSize;
    }
    Logger::log(LogLevel::Info,
                "Loaded inputImage from " + std::string(inputImage) + " with size " +
                std::to_string(this->inputImage.width()) + "x" +
                std::to_string(this->inputImage.height()));
    Logger::log(LogLevel::Info, "Pattern size set to: " + std::to_string(options.patternSize));
    Logger::log(LogLevel::Info, "Output size set to: " + std::to_string(outputSize));
}

bool WFC::prepareWFC(bool savePatterns, const std::string &path) {
    bool success = generatePatterns();
    if (!success) {
        return false;
    }
    generateOffsets();
    generateRules();
    logRules();
    //initialize coeff matrix to be outputSize x outputSize x unique patterns count
    coeffMatrix = std::vector<std::vector<std::vector<bool>>>(
            outputSize, std::vector<std::vector<bool>>(
                    outputSize, std::vector<bool>(
                            patterns.size(), True)
            ));
    //initialize collapsed tiles to be outputSize x outputSize with invalid value
    collapsedTiles = std::vector<std::vector<int>>(outputSize, std::vector<int>(outputSize, -1));
    calculateProbabilities();
    if (savePatterns) {
        generatePatternImagePreview(path);
    }
    startWFC();
    return success;
}

bool WFC::generatePatterns() {
    Timer timer("generatePatterns");
    size_t totalPatterns = 0;
    for (size_t x = 0; x <= inputImage.width() - options.patternSize; x++) {
        for (size_t y = 0; y <= inputImage.height() - options.patternSize; y++) {
            cimg::CImg<unsigned char> pattern = inputImage.get_crop(x, y, x + options.patternSize - 1,
                                                                    y + options.patternSize - 1);
            totalPatterns++;
            addPattern(pattern);

            if (options.flip) {
                addPattern(pattern.get_mirror('x'));
                addPattern(pattern.get_mirror('y'));
                totalPatterns += 2;
            }

            if (options.rotate) {
                for (int angle = 90; angle <= 270; angle += 90) {
                    addPattern(pattern.get_rotate(static_cast<float>(angle)));
                    totalPatterns += 3;
                }
            }
        }
    }
    Logger::log(LogLevel::Info, "Total possible patterns:  " + std::to_string(totalPatterns));
    Logger::log(LogLevel::Info, "Generated " + std::to_string(patterns.size()) + " patterns");

    return true;
}

void WFC::addPattern(const cimg::CImg<unsigned char> &pattern) {
    std::string patternStr = patternToStr(pattern);
    if (patternFrequency.find(patternStr) == patternFrequency.end()) {
        patternFrequency[patternStr] = 1;
        patterns.push_back(pattern);
    } else {
        patternFrequency[patternStr]++;
    }
}

std::string WFC::patternToStr(const cimg::CImg<unsigned char> &pattern) const {
    std::string patternKey;
    patternKey.reserve(pattern.width() * pattern.height() * pattern.spectrum() * 4);
    for (int x = 0; x < pattern.width(); ++x) {
        for (int y = 0; y < pattern.height(); ++y) {
            for (int c = 0; c < pattern.spectrum(); ++c) {
                patternKey += std::to_string(pattern(x, y, 0, c)) + ",";
            }
        }
    }
    return patternKey;
}

void WFC::generatePatternImagePreview(const std::string &path) {
    Logger::log(LogLevel::Info, "Generating pattern image preview");
    Timer timer("generatePatternImagePreview");
    size_t scaledPatternSize = options.scale * options.patternSize;
    auto [rows, cols] = getPatternGridSize();
    auto sb = options.spaceBetween;
    size_t imageSizeWithSpaceX = ((scaledPatternSize + sb) * rows) + sb;
    size_t imageSizeWithSpaceY = ((scaledPatternSize + sb) * cols) + sb;

    generatedPatternsImage = cimg::CImg<unsigned char>(imageSizeWithSpaceX, imageSizeWithSpaceY, 1, 3, 0);
    unsigned char color[] = {0, 0, 0}; // white color for the grid
    generatedPatternsImage.fill(128, 128, 128);
    for (size_t i = 0; i < patterns.size(); i++) {
        size_t row = i / cols;
        size_t col = i % cols;
        //need to make copy, resize will modify original pattern
        cimg::CImg<unsigned char> resizedPattern = patterns[i].get_resize(scaledPatternSize, scaledPatternSize);
        generatedPatternsImage.draw_image(sb + (row * scaledPatternSize) + (sb * row),
                                          sb + (col * scaledPatternSize) + (sb * col),
                                          resizedPattern);


        std::stringstream ss;
        ss << "#:" << std::to_string(i) <<
           " F:" << std::to_string(patternFrequency[patternToStr(patterns.at(i))]) <<
           " P:" << std::fixed << std::setprecision(2) << probabilities.at(i) * 100 << "%%";

        generatedPatternsImage.draw_text(sb + (row * scaledPatternSize) + (sb * row),
                                         sb + (col * scaledPatternSize) + (sb * col) + scaledPatternSize,
                                         ss.str().c_str(), color, 0, 1, 15);
    }


    // Draw grid, prepare it first
    for (size_t row = 0; row < rows; row++) {
        for (size_t col = 0; col < cols; col++) {
            for (size_t i = 0; i <= options.patternSize; i++) {
                size_t patternIndex = row * cols + col;
                if (patternIndex >= patterns.size()) {
                    continue;
                }
                cimg::CImg<unsigned char> pattern = patterns[row * cols + col];

                size_t gridOffset = i * options.scale;
                generatedPatternsImage.draw_line(sb + (row * scaledPatternSize) + (sb * row),
                                                 sb + (col * scaledPatternSize) + (sb * col) + gridOffset,
                                                 sb + (row * scaledPatternSize) + (sb * row) + scaledPatternSize,
                                                 sb + (col * scaledPatternSize) + (sb * col) + gridOffset,
                                                 color);
                generatedPatternsImage.draw_line(sb + (row * scaledPatternSize) + (sb * row) + gridOffset,
                                                 sb + (col * scaledPatternSize) + (sb * col),
                                                 sb + (row * scaledPatternSize) + (sb * row) + gridOffset,
                                                 sb + (col * scaledPatternSize) + (sb * col) + scaledPatternSize,
                                                 color);
            }
        }
    }

    generatedPatternsImage.save_png(path.c_str());
}

std::tuple<size_t, size_t> WFC::getPatternGridSize() {
    size_t patternsSize = patterns.size();
    auto sqrtPatternsCount = static_cast<unsigned int>(std::sqrt(patternsSize));
    unsigned int rows = sqrtPatternsCount;
    unsigned int cols = sqrtPatternsCount;
    while (rows * cols < patternsSize) {
        if (rows < cols) {
            rows++;
        } else {
            cols++;
        }
    }
    return {rows, cols};
}

void WFC::generateOffsets() {
    int size = options.patternSize - 1;
    offsets.reserve((2 * size + 1) * (2 * size + 1) - 1);
    for (int i = -size; i <= size; i++) {
        for (int j = -size; j <= size; j++) {
            if (i == 0 && j == 0) {
                continue;
            }
            offsets.emplace_back(i, j);
        }
    }
}

void WFC::generateRules() {
    Timer timer("generateRules");
    //iterate over all patterns
    rules.resize(patterns.size());
    for (size_t i = 0; i < patterns.size(); i++) {
        // iterate all offsets
        for (const auto &offset: offsets) {
            // iterate all patterns again to match against them
            for (size_t j = i; j < patterns.size(); j++) {
                if (checkForMatch(patterns[i], patterns[j], offset)) {
                    rules[i][offset].insert(j);
                    rules[j][Point(-offset.x, -offset.y)].insert(i);
                }
            }
        }
    }
}

bool WFC::checkForMatch(const cimg::CImg<unsigned char> &p1, const cimg::CImg<unsigned char> &p2,
                        const Point &offset) const {
    cimg::CImg<unsigned char> p1Offset = maskWithOffset(p1, offset);
    cimg::CImg<unsigned char> p2Offset = maskWithOffset(p2, Point(-offset.x, -offset.y));
    return p1Offset == p2Offset;
}

cimg::CImg<unsigned char> WFC::maskWithOffset(const cimg::CImg<unsigned char> &pattern, const Point &offset) const {
    //check bounds
    if (abs(offset.x) > pattern.width() || abs(offset.y) > pattern.height()) {
        return {};
    }
    //logPrettyPatternData(pattern);
    Point p1 = {std::max(0, offset.x), std::max(0, offset.y)};
    Point p2 = {std::min(pattern.width() - 1, pattern.width() - 1 + offset.x),
                std::min(pattern.height() - 1, pattern.height() - 1 + offset.y)};;

    auto crop = pattern.get_crop(p1.x, p1.y, p2.x, p2.y);
    //logPrettyPatternData(crop);
    return crop;
}

void WFC::calculateProbabilities() {
    //calculate sum of all frequencies
    sumFrequency = std::accumulate(patternFrequency.begin(), patternFrequency.end(), 0.0,
                                   [](double acc, const auto &p) {
                                       return acc + p.second;
                                   });
    //calculate probabilities for each pattern
    probabilities.resize(patternFrequency.size());
    auto it = patternFrequency.begin();
    std::transform(it, patternFrequency.end(), probabilities.begin(),
                   [this](const auto &pair) {
                       return static_cast<double>(pair.second) / sumFrequency;
                   });
}

void WFC::startWFC() {
    Timer timer("startWFC");
    status = WFCStatus::RUNNING;
    //points because we need to know the position and reference to the coeffMatrix would not give us that
    //std::priority_queue<Point, std::vector<Point>, CompareEntropy> entropyQueue(CompareEntropy(*this));
    //point we start from
    //entropyQueue.emplace(static_cast<int>(outputSize / 2), static_cast<int>(outputSize / 2));
    size_t iterations = 0;
    while (status == WFCStatus::RUNNING) {
        Logger::log(LogLevel::Debug, "Iteration: " + std::to_string(iterations));
        LogCoeffMatrix();

        //logEntropyQueue();

        //find cell with lowest entropy
        auto lowestEntropy = Observe();

        if (status == WFCStatus::CONTRADICTION) {
            Logger::log(LogLevel::Info, "Contradiction found");
            break;
        }
        if (status == WFCStatus::SOLUTION) {
            Logger::log(LogLevel::Info, "Solution found");
            displayOutputImage("../outputs/solution.png", outputSize * options.scale, outputSize * options.scale);
            break;
        }

        propagate(lowestEntropy);
        iterations++;
    }
    Logger::log(LogLevel::Info, "WFC finished in " + std::to_string(iterations) + " iterations");

}

Point WFC::Observe() {
    //calculate entropy for each cell
    std::vector<std::vector<double>> entropyMatrix(outputSize, std::vector<double>(outputSize, 0.0));
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
    /*Logger::log(LogLevel::Debug,
                "Min entropy at " + std::to_string(minEntropyPoint.x) + " " + std::to_string(minEntropyPoint.y) +
                " is " + std::to_string(entropyMatrix[minEntropyPoint.y][minEntropyPoint.x]));*/
    collapseCell(minEntropyPoint, minEntropyProbabilities);

    return minEntropyPoint;
}

void WFC::fillEntropyMatrix(std::vector<std::vector<double>> &entropyMatrix) {
    for (size_t i = 0; i < outputSize; ++i) {
        for (size_t j = 0; j < outputSize; ++j) {
            for (size_t k = 0; k < coeffMatrix[i][j].size(); ++k) {
                entropyMatrix[i][j] = getShannonEntropy(Point(j, i));
                /*if (coeffMatrix[i][j][k]) {
                    entropyMatrix[i][j] += probabilities[k];
                }*/
            }
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
        if (coeffMatrix[p.y][p.x][option]) {
            double weight = probabilities[option];
            sumWeights += weight;
            sumLogWeights += weight * std::log(weight);
        }
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
    if(minPoint.x == -1 && minPoint.y == -1) {
        LogCoeffMatrix();
        status = WFCStatus::SOLUTION;
        return {minPoint, {}};
    }

    //finds all possible points with that min entropy
    std::vector<Point> minEntropyPositions;
    for(int i = 0; i < entropyMatrix.size(); i++) {
        for(int j = 0; j < entropyMatrix[i].size(); j++) {
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
        minEntropyProbabilities[k] = coeffMatrix[minPoint.y][minPoint.x][k] ? probabilities[k] : 0.0;
    }

    return {minPoint, minEntropyProbabilities};
}

void WFC::collapseCell(const Point &p, std::vector<double> &probabilities) {
    //choose a random possible option from probabilities vector and mark that chosen index in collapsedTiles
    std::discrete_distribution<size_t> dist(probabilities.begin(), probabilities.end());
    size_t chosenPattern = dist(rng);
    collapsedTiles[p.y][p.x] = chosenPattern;

    for (size_t i = 0; i < coeffMatrix[p.y][p.x].size(); ++i) {
        coeffMatrix[p.y][p.x][i] = (i == chosenPattern);
    }
}

void
WFC::propagate(Point &minEntropyPoint) {
    std::deque<Point> propagationQueue;
    propagationQueue.push_back(minEntropyPoint);
    size_t propagations_count = 0;
    while (!propagationQueue.empty()) {
        Point currentPoint = propagationQueue.front();
        propagationQueue.pop_front();
        LogPossibleRulesForPoint(currentPoint);
        for (auto &offset: offsets) {
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
                bool alreadyInQueue = std::find(propagationQueue.begin(), propagationQueue.end(), neighborPoint) !=
                                      propagationQueue.end();
                if (!alreadyInQueue) {
                    propagationQueue.push_back(neighborPoint);
                }
            }
        }
        propagations_count++;
    }
}

std::pair<bool, bool> WFC::updateCell(const Point &current, const Point &neighbour, const Point &offset) {
    //patterns for current and neighbour cell
    std::vector<bool> &currentPatterns = coeffMatrix[current.y][current.x];
    std::vector<bool> &neighbourPatterns = coeffMatrix[neighbour.y][neighbour.x];

    std::vector<bool> possiblePatternsInOffset(neighbourPatterns.size(), false);
    for (size_t i = 0; i < currentPatterns.size(); i++) {
        if (!currentPatterns[i]) {
            continue;
        }
        for (const auto &possiblePattern: rules[i][offset]) {
            possiblePatternsInOffset[possiblePattern] = true;
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
    return p.x >= 0 && p.y >= 0 && p.x < outputSize && p.y < outputSize;
}

void WFC::logRules() const {
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
        Logger::log(LogLevel::Debug,  "Total number of rules for pattern " + std::to_string(i) + " is " +
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

void WFC::logPrettyPatternData(const cimg::CImg<unsigned char> &pattern) const {
    std::string patternData;
    for (int x = 0; x < pattern.width(); x++) {
        patternData += "[ ";
        for (int y = 0; y < pattern.height(); y++) {
            patternData += std::to_string(pattern(x, y, 0)) + " ";
        }
        patternData += "]\n";
    }
    Logger::log(LogLevel::Debug, "Pattern data: \n" + patternData);
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
    Logger::log(LogLevel::Debug,
                "Possible patterns at point (" + std::to_string(point.x) + ", " + std::to_string(point.y) + ") is [ " +
                possiblePetternsStr + "] ");

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
    std::string path =  "../outputs/iterations/iteration" + std::to_string(index) + ".png";
    displayOutputImage(path, 128, 128);
    index++;
}

void WFC::displayOutputImage(const std::string &path, int width, int height) const {
    cimg_library::CImg<unsigned char> res(coeffMatrix.size(), coeffMatrix[0].size(), 1, 3, 0);

    for (int row = 0; row < coeffMatrix.size(); ++row) {
        for (int col = 0; col < coeffMatrix[row].size(); ++col) {
            std::vector<cimg_library::CImg<unsigned char>> validPatterns;
            for (int pattern = 0; pattern < patterns.size(); ++pattern) {
                if (coeffMatrix[row][col][pattern]) {
                    validPatterns.emplace_back(patterns[pattern]);
                }
            }

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
