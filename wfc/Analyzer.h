//
// Created by Jakub on 19.04.2024.
//

#ifndef WFC_ANALYZER_H
#define WFC_ANALYZER_H

#include <cstddef>
#include <set>
#include <numeric>
#include <iomanip>

//CImg configuration
#define cimg_use_png
#define cimg_display_png

#include "../CImg.h"
#include "../utility/Logger.h"
#include "../utility/Timer.h"
#include "../utility/Point.h"
#include "../utility/FileUtil.h"

namespace cimg = cimg_library;

namespace WFC {

    struct AnalyzerOptions {
        size_t patternSize;
        int previewImgScale;
        int spaceBetween;
        bool rotate;
        bool flip;
    };

    using Rules = std::vector<std::unordered_map<Util::Point, std::set<size_t>, Util::PointHash>>;

    class Analyzer {
    public:
        Analyzer(AnalyzerOptions &options, std::string_view pathToInputImage);

        //analyzes the image and produces the resources for WFC algorithm
        void analyze();

        const AnalyzerOptions &getOptions() const;

        const std::vector<cimg::CImg<unsigned char>> &getPatterns() const;

        void savePatternsPreviewTo(const std::string &path);

        const std::vector<double> &getProbabilities() const;

        const std::vector<Util::Point> &getOffsets() const;

        const Rules &getRules() const;

        void setOptions(const AnalyzerOptions &options);

    private:
        void generatePatterns();

        void generateOffsets();

        void generateRules();

        void logRules();

        void addPattern(const cimg::CImg<unsigned char> &pattern);

        bool
        checkForMatch(const cimg::CImg<unsigned char> &p1, const cimg::CImg<unsigned char> &p2,
                      const Util::Point &offset) const;

        cimg::CImg<unsigned char> maskWithOffset(const cimg::CImg<unsigned char> &pattern, const Util::Point &offset) const;

        std::string patternToStr(const cimg::CImg<unsigned char> &pattern) const;

        void calculateProbabilities();

        std::tuple<size_t, size_t> getPatternGridSize();

        void LogProbabilities();

    private:
        cimg::CImg<unsigned char> inputImage;
        AnalyzerOptions options;

        //input image patterns are extracted from
        const cimg::CImg<unsigned char> &getInputImage() const;

        //vector of unique extracted from input image
        std::vector<cimg::CImg<unsigned char>> patterns;
        //map to store frequency of each pattern
        std::unordered_map<std::string, int> patternFrequency;
        //vector of all patterns that store map of their offsets with possible neighbors at that offset
        Rules rules;
        //vector of all offsets
        std::vector<Util::Point> offsets;
        double sumFrequency{};
        std::vector<double> probabilities;
    };
}
#endif //WFC_ANALYZER_H
