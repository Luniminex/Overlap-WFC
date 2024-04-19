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
