#include "wfc/WFC.h"
#include "utility/Logger.h"
#include "CImg.h"

int main() {
    freopen("../output.txt", "w", stdout);

    Logger::setLogLevel(LogLevel::Debug);
    AnalyzerOptions analyzerOptions = {3, 64, 16, true, true};
    BacktrackerOptions backtrackerOptions = {100, 3, true};
    WFC wfc("../resources/Flowers.png", analyzerOptions, backtrackerOptions, 8, 16);
    if(wfc.prepareWFC(true)){
        Logger::log(LogLevel::Info, "WFC prepared successfully");
        wfc.startWFC(true);
    } else {
        Logger::log(LogLevel::Error, "WFC preparation failed");
        return 1;
    }
    return 0;
}
