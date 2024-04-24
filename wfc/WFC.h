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

enum class WFCStatus {
    RUNNING,
    CONTRADICTION,
    SOLUTION,
    PREPARING,
};

class WFC {
public:
    WFC(const std::string_view &pathToInputImage, AnalyzerOptions &options);
    bool prepareWFC(bool savePatterns = false, const std::string &path = "../outputs/patterns/generatedPatterns.png");
private:
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
    std::vector<double> getProbabilities(const Point &cell) const;

private:
    Analyzer analyzer;
    //matrix to store possible patterns at each position
    std::vector<std::vector<std::vector<bool>>> coeffMatrix;
    //this is gonna get removed probably in future
    std::vector<std::vector<int>> collapsedTiles;
    std::mt19937 rng;

    cimg::CImg<unsigned char> outputImage;

    WFCStatus status;
};


#endif //WFC_WFC_H
