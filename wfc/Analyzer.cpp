//
// Created by Jakub on 19.04.2024.
//

#include "Analyzer.h"


Analyzer::Analyzer(AnalyzerOptions &options, const std::string_view pathToInputImage) :
options(options)
{
    this->inputImage = cimg::CImg<unsigned char>(pathToInputImage.data());
    checkPatternSize();
    Logger::log(LogLevel::Info,
                "Loaded pathToInputImage from " + std::string(pathToInputImage) + " with size " +
                std::to_string(this->inputImage.width()) + "x" +
                std::to_string(this->inputImage.height()));

    Logger::log(LogLevel::Info, "Pattern size set to: " + std::to_string(options.patternSize));
    Logger::log(LogLevel::Info, "Output size set to: " + std::to_string(options.outputSize));
}

const cimg::CImg<unsigned char>& Analyzer::getInputImage() const {
    return inputImage;
}

bool Analyzer::checkPatternSize() {
    if (options.patternSize > options.outputSize) {
        Logger::log(LogLevel::Warning, "Pattern size cannot be greater than output size");
        return false;
    }
    return true;
}

bool Analyzer::analyze() {
    bool success = generatePatterns();
    if (!success) {
        return false;
    }
    generateOffsets();
    generateRules();
    logRules();
    calculateProbabilities();
    return success;
}

bool Analyzer::generatePatterns(){
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

void Analyzer::generateOffsets() {
    int size = static_cast<int>(options.patternSize - 1);
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

void Analyzer::generateRules() {
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

bool Analyzer::checkForMatch(const cimg::CImg<unsigned char> &p1, const cimg::CImg<unsigned char> &p2,
                        const Point &offset) const {
    cimg::CImg<unsigned char> p1Offset = maskWithOffset(p1, offset);
    cimg::CImg<unsigned char> p2Offset = maskWithOffset(p2, Point(-offset.x, -offset.y));
    return p1Offset == p2Offset;
}

cimg::CImg<unsigned char> Analyzer::maskWithOffset(const cimg::CImg<unsigned char> &pattern, const Point &offset) const {
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

void Analyzer::logRules() {
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

void Analyzer::addPattern(const cimg::CImg<unsigned char> &pattern) {
    std::string patternStr = patternToStr(pattern);
    if (patternFrequency.find(patternStr) == patternFrequency.end()) {
        patternFrequency[patternStr] = 1;
        patterns.push_back(pattern);
    } else {
        patternFrequency[patternStr]++;
    }
}

std::string Analyzer::patternToStr(const cimg::CImg<unsigned char> &pattern) const {
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

void Analyzer::calculateProbabilities() {
    // Calculate sum of all frequencies
    sumFrequency = std::accumulate(patternFrequency.begin(), patternFrequency.end(), 0.0,
                                   [](double acc, const auto &p) {
                                       return acc + p.second;
                                   });

    // Resize the probabilities vector to match the size of the patterns vector
    probabilities.resize(patterns.size());

    // Calculate probabilities for each pattern
    for (size_t i = 0; i < patterns.size(); ++i) {
        std::string patternStr = patternToStr(patterns[i]);
        double frequency = patternFrequency[patternStr];
        double probability = frequency / sumFrequency;
        probabilities[i] = probability;
    }

    LogProbabilities();
}

void Analyzer::savePatternsPreviewTo(const std::string &path) {
    Logger::log(LogLevel::Info, "Generating pattern image preview");
    Timer timer("savePatternsPreviewTo");
    size_t scaledPatternSize = options.scale * options.patternSize;
    auto [rows, cols] = getPatternGridSize();
    auto sb = options.spaceBetween;
    size_t imageSizeWithSpaceX = ((scaledPatternSize + sb) * rows) + sb;
    size_t imageSizeWithSpaceY = ((scaledPatternSize + sb) * cols) + sb;

    auto generatedPatternsImage = cimg::CImg<unsigned char>(imageSizeWithSpaceX, imageSizeWithSpaceY, 1, 3, 0);
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
        std::string patternStr = patternToStr(patterns.at(i));
        ss << "#:" << std::to_string(i) <<
           " F:" << std::to_string(patternFrequency[patternStr]) <<
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

std::tuple<size_t, size_t> Analyzer::getPatternGridSize() {
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

const AnalyzerOptions &Analyzer::getOptions() const {
    return options;
}

const std::vector<cimg::CImg<unsigned char>> &Analyzer::getPatterns() const {
    return patterns;
}

size_t Analyzer::getOutputSize() const {
    return options.outputSize;
}

const std::vector<double>& Analyzer::getProbs() const {
    return probabilities;
}

const std::vector<Point> &Analyzer::getOffsets() const {
    return offsets;
}

const Rules &Analyzer::getRules() const {
    return rules;
}

void Analyzer::LogProbabilities() {
    //sum up all probabilities
    double sum = std::accumulate(probabilities.begin(), probabilities.end(), 0.0);
    Logger::log(LogLevel::Debug, "Sum of all probabilities: " + std::to_string(sum));

    //match pattern to its probability and print it via Logger
    for (size_t i = 0; i < patterns.size(); ++i) {
        std::string patternStr = patternToStr(patterns[i]);
        Logger::log(LogLevel::Debug, "Pattern " + patternStr + " has probability: " +
                                     std::to_string(probabilities[i]));
    }
}