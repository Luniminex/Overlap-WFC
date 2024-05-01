#include "wfc/WFC.h"
#include "utility/Logger.h"
#include "cxxopts.hpp"

int main(int argc, char *argv[]) {
    cxxopts::Options options("WFC", "Wave Function Collapse CLI");
    options.add_options()
            ("i,input", "Input image path", cxxopts::value<std::string>()->default_value("../resources/Cave.png"))
            ("p,pattern", "Pattern size", cxxopts::value<int>()->default_value("3"))
            ("s,scale", "Preview image scale", cxxopts::value<int>()->default_value("64"))
            ("b,space", "Space between", cxxopts::value<int>()->default_value("16"))
            ("r,rotate", "Enable rotation", cxxopts::value<bool>()->default_value("true"))
            ("f,flip", "Enable flip", cxxopts::value<bool>()->default_value("true"))
            ("d,depth", "Backtracker depth", cxxopts::value<int>()->default_value("50"))
            ("m,max_iterations", "Backtracker max iterations", cxxopts::value<int>()->default_value("3"))
            ("e,enable", "Enable backtracker", cxxopts::value<bool>()->default_value("true"))
            ("w,width", "Output width", cxxopts::value<int>()->default_value("16"))
            ("h,height", "Output height", cxxopts::value<int>()->default_value("16"))
            ("l,log", "Log file", cxxopts::value<std::string>()->default_value("../wfc.log"))
            ("v,verbose", "Verbose logging", cxxopts::value<bool>()->default_value("false"))
            ("o,output", "Output image path", cxxopts::value<std::string>()->default_value("../outputs/solution.png"
            ))
            ("a,iterations", "Iterations directory",
             cxxopts::value<std::string>()->default_value("../outputs/iterations/"
             ))
            ("t,patterns", "Generated patterns directory",
             cxxopts::value<std::string>()->default_value("../outputs/patterns/generated-patterns.png"
             ))
            ("c,failed", "Failed output image path",
             cxxopts::value<std::string>()->default_value("../outputs/failed-solution.png"
             ))
            ("h,help", "CLI for running WFC algorithm");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if(result.count("verbose")) {
        Logger::setLogLevel(LogLevel::Debug);
    } else {
        Logger::setLogLevel(LogLevel::Info);
    }

    std::string option_value = result["option_name"].as<std::string>();
    freopen(option_value.c_str(), "w", stdout);

    AnalyzerOptions analyzerOptions = {
            static_cast<size_t>(result["pattern"].as<int>()),
            result["scale"].as<int>(),
            result["space"].as<int>(),
            result["rotate"].as<bool>(),
            result["flip"].as<bool>()
    };

    BacktrackerOptions backtrackerOptions = {
            static_cast<unsigned int>(result["depth"].as<int>()),
            static_cast<unsigned int>(result["max_iterations"].as<int>()),
            result["enable"].as<bool>()
    };

    WFC wfc(result["input"].as<std::string>(),
            analyzerOptions,
            backtrackerOptions,
            result["width"].as<int>(),
            result["height"].as<int>());

    wfc.setSavePaths({
                             result["patterns"].as<std::string>(),
                             result["output"].as<std::string>(),
                             result["failed"].as<std::string>(),
                             result["iterations"].as<std::string>()
                     });

    if (wfc.prepareWFC(true)) {
        Logger::log(LogLevel::Info, "WFC prepared successfully");
        wfc.startWFC(true);
    } else {
        Logger::log(LogLevel::Error, "WFC preparation failed");
        return 1;
    }

    return 0;
}