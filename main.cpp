#include "wfc/WFC.h"
#include "utility/Logger.h"
#include "CImg.h"

int main() {
    //freopen("../output.txt", "w", stdout);

    Logger::setLogLevel(LogLevel::Info);

    Point size = {16, 16};
    WFC wfc("../resources/image.png", 16,
            {64, 2, 16, true, true});
    std::cout << wfc.prepareWFC(true) << std::endl;

    cimg_library::CImg<unsigned char> image(16, 16, 1, 3);

    return 0;
}
