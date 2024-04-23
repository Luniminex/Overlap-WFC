#include "wfc/WFC.h"
#include "utility/Logger.h"
#include "CImg.h"

int main() {
    freopen("../output.txt", "w", stdout);

    Logger::setLogLevel(LogLevel::Debug);
    AnalyzerOptions options = {64, 3, 16, true, true, 64};
    Point size = {16, 16};
    WFC wfc("../resources/ColoredCity.png", options);
    std::cout << wfc.prepareWFC(true) << std::endl;

    cimg_library::CImg<unsigned char> image(16, 16, 1, 3);

    return 0;
}
