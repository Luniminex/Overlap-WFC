//
// Created by Jakub on 10.03.2024.
//

/*
 * improvemenets: make the comparison functors lambdas
 */

#ifndef WFC_WFC_H
#define WFC_WFC_H

#include <string>
#include <queue>
#include <numeric>
#include <random>
#include <iomanip>
#include <sstream>

#include "Analyzer.h"
#include "Backtracker.h"

enum class WFCStatus {
    RUNNING,
    CONTRADICTION,
    SOLUTION,
    PREPARING,
};

class WFC {
public:
    WFC(const std::string_view &pathToInputImage, AnalyzerOptions &options, BacktrackerOptions &backtrackerOptions, size_t width, size_t height);
    bool prepareWFC(bool savePatterns = false, const std::string &path = "../outputs/patterns/generatedPatterns.png");
    bool startWFC(bool saveIterations = false, const std::string &dir = "../outputs/iterations/");
    void setAnalyzerOptions(const AnalyzerOptions &options);
    void enableBacktracker();
    void disableBacktracker();
    void setBacktrackerOptions(const BacktrackerOptions &options);
private:
    void fillEntropyMatrix(std::vector<std::vector<double>>& entropyMatrix);
    Point Observe();
    bool checkForContradiction() const;
    double getShannonEntropy(const Point &p) const;
    std::pair<Point, std::vector<double>>
    findMinEntropyPoint( std::vector<std::vector<double>>& entropyMatrix);
    void collapseCell(const Point &p, std::vector<double> &probabilities);
    void propagate(Point &minEntropyPoint);
    std::pair<bool, bool> updateCell(const Point& current, const Point& neighbour, const Point& offset);
    Point wrapPoint(const Point &p, const Point& offset) const;
    bool isPointValid(const Point &p) const;
    void displayOutputImage(const std::string& path) const;
    void logEntropyMatrix(const std::vector<std::vector<double>>& entropyMatrix) const;
    void logState();
private:
    Analyzer analyzer;
    Backtracker backtracker;
    State state;
    std::mt19937 rng;
    WFCStatus status;
    cimg::CImg<unsigned char> outputImage;
    size_t outWidth;
    size_t outHeight;
};


#endif //WFC_WFC_H
