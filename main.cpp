#include "wfc/WFC.h"
#include "utility/Logger.h"
#include "CImg.h"

int main() {
    freopen("../output.txt", "w", stdout);

    Logger::setLogLevel(LogLevel::Debug);
    AnalyzerOptions options = {64, 3, 16, false, true, 16};
    WFC wfc("../resources/Flowers.png", options);
    std::cout << wfc.prepareWFC(true) << std::endl;
    return 0;
}
