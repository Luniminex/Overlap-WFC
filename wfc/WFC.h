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
#include "../utility/FileUtil.h"

namespace WFC {

    enum class WFCStatus {
        RUNNING,
        CONTRADICTION,
        SOLUTION,
        PREPARING,
    };

    struct WFCSavePaths {
        std::string generatedPatternsDir;
        std::string outputImageDir;
        std::string failedOutputImageDir;
        std::string iterationsDir;
        bool savePatterns;
    };

    class WFC {
    public:
        WFC(const std::string_view &pathToInputImage, AnalyzerOptions &options, BacktrackerOptions &backtrackerOptions,
            size_t width, size_t height);

        void prepareWFC();

        bool startWFC();

        void setSavePaths(const WFCSavePaths &paths);

        void setAnalyzerOptions(const AnalyzerOptions &options);

        void enableBacktracker();

        void disableBacktracker();

        void setBacktrackerOptions(const BacktrackerOptions &options);

        void saveOutput() const;

    private:
        void createDirectories();

        void fillEntropyMatrix(std::vector<std::vector<double>> &entropyMatrix);

        Util::Point Observe();

        bool checkForContradiction() const;

        double getShannonEntropy(const Util::Point &p) const;

        std::pair<Util::Point, std::vector<double>>
        findMinEntropyPoint(std::vector<std::vector<double>> &entropyMatrix);

        void collapseCell(const Util::Point &p, std::vector<double> &probabilities);

        void propagate(Util::Point &minEntropyPoint);

        std::pair<bool, bool> updateCell(const Util::Point &current, const Util::Point &neighbour, const Util::Point &offset);

        Util::Point wrapPoint(const Util::Point &p, const Util::Point &offset) const;

        bool isPointValid(const Util::Point &p) const;

        void displayOutputImage(const std::string &dir, const std::string &fileName, bool checkForUniqueFilename) const;

        void logEntropyMatrix(const std::vector<std::vector<double>> &entropyMatrix) const;

        void logState();

        void saveOutputImage();

    private:
        Analyzer analyzer;
        Backtracker backtracker;
        State state;
        cimg::CImg<unsigned char> outputImage;
        std::mt19937 rng;
        WFCSavePaths savePaths;
        WFCStatus status;
        size_t outWidth;
        size_t outHeight;
    };

}
#endif //WFC_WFC_H
