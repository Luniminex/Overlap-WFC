#include "wfc/WFC.h"
#include "utility/Logger.h"
#include "CImg.h"

int main() {
    //freopen("../output.txt", "w", stdout);

    Logger::setLogLevel(LogLevel::Debug);
    AnalyzerOptions options = {64, 2, 16, true, false, 16};
    Point size = {16, 16};
    WFC wfc("../resources/image.png", options);
    std::cout << wfc.prepareWFC(true) << std::endl;

    cimg_library::CImg<unsigned char> image(16, 16, 1, 3);

    return 0;
}
