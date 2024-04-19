//
// Created by Jakub on 10.03.2024.
//

/*
 * improvemenets: make the comparison functors lambdas
 */

#ifndef WFC_WFC_H
#define WFC_WFC_H

#include <string>
#include <set>
#include <queue>
#include <numeric>
#include <random>
#include <iomanip>
#include <sstream>

#define cimg_use_png
#define cimg_display_png

#include "../CImg.h"
#include "../utility/Logger.h"
#include "../utility/Timer.h"

namespace cimg = cimg_library;

struct Point {
    int x;
    int y;

    Point(int x, int y) : x(x), y(y) {};

    bool operator==(const Point &other) const {
        return x == other.x && y == other.y;
    }

    Point operator+(const Point &other) const {
        return {x + other.x, y + other.y};
    }

    Point operator-(const Point &other) const {
        return {x - other.x, y - other.y};
    }
};

struct PointHash {
    std::size_t operator()(const Point &p) const {
        return std::hash<int>()(p.x) ^ std::hash<int>()(p.y);
    }
};

struct AnalyzerOptions {
    int scale;
    size_t patternSize;
    int spaceBetween;
    bool rotate;
    bool flip;
};

enum class WFCStatus {
    RUNNING,
    CONTRADICTION,
    SOLUTION,
    PREPARING,
};

class WFC {
public:
    WFC(const std::string_view &inputImage, const size_t &outputSize, const AnalyzerOptions &options);

    bool prepareWFC(bool savePatterns = false, const std::string &path = "../outputs/patterns/generatedPatterns.png");

private:
    bool generatePatterns();

    void addPattern(const cimg::CImg<unsigned char> &pattern);

    std::string patternToStr(const cimg::CImg<unsigned char> &pattern) const;

    //generates preview of generated patterns
    void generatePatternImagePreview(const std::string &path);

    std::tuple<size_t, size_t> getPatternGridSize();

    void generateOffsets();

    void generateRules();

    void logPrettyPatternData(const cimg::CImg<unsigned char> &pattern) const;

    bool checkForMatch(const cimg::CImg<unsigned char> &p1, const cimg::CImg<unsigned char> &p2, const Point &offset) const;

    cimg::CImg<unsigned char> maskWithOffset(const cimg::CImg<unsigned char> &pattern, const Point &offset) const;

    void calculateProbabilities();

    void startWFC();

    void fillEntropyMatrix(std::vector<std::vector<double>>& entropyMatrix);

    Point Observe();

    bool checkForContradiction() const;

    size_t getEntropy(Point p) const;

    double getShannonEntropy(const Point &p) const;

    std::pair<Point, std::vector<double>>
    findMinEntropyPoint( std::vector<std::vector<double>>& entropyMatrix);

    void collapseCell(const Point &p, std::vector<double> &probabilities);

    void propagate(Point &minEntropyPoint);

    std::pair<bool, bool> updateCell(const Point& current, const Point& neighbour, const Point& offset);

    Point wrapPoint(const Point &p, const Point& offset) const;

    bool isPointValid(const Point &p) const;

    void logRules() const;

    void LogPossibleRulesForPoint(Point point) const;

    void displayOutputImage(const std::string& path, int width, int height) const;

    void logEntropyMatrix(const std::vector<std::vector<double>>& entropyMatrix) const;

    void LogCoeffMatrix() const;
private:
    AnalyzerOptions options{};
    //vector of unique extracted from input image
    std::vector<cimg::CImg<unsigned char>> patterns;
    //map to store frequency of each pattern
    std::unordered_map<std::string, int> patternFrequency;
    //vector of all patterns that store map of their offsets with possible neighbors at that offset
    std::vector<std::unordered_map<Point, std::set<size_t>, PointHash>> rules;
    //matrix to store possible patterns at each position
    std::vector<std::vector<std::vector<bool>>> coeffMatrix;
    //this is gonna get removed probably in future
    std::vector<std::vector<int>> collapsedTiles;
    //vector of all offsets
    std::vector<Point> offsets;
    std::mt19937 rng;
    cimg::CImg<unsigned char> inputImage;
    cimg::CImg<unsigned char> outputImage;
    //preview of generated patterns
    cimg::CImg<unsigned char> generatedPatternsImage;
    double sumFrequency;
    std::vector<double> probabilities;

    size_t outputSize;

    WFCStatus status;
};


#endif //WFC_WFC_H
