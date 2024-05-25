#include <thread>
#include <mutex>
#include "wfc/WFC.h"
#include "utility/Logger.h"
#include "utility/CLI.h"
#include "cxxopts.hpp"

bool checkIfHelp(cxxopts::ParseResult &result, cxxopts::Options &options) {
    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return true;
    }
    return false;
}

void setVerbose(cxxopts::ParseResult &result) {
    if (result.count("verbose")) {
        Util::Logger::setLogLevel(Util::LogLevel::Debug);
    } else {
        Util::Logger::setLogLevel(Util::LogLevel::Warning);
    }
}

void setLogFile(cxxopts::ParseResult &result) {
    std::string useLogFile = result["log"].as<std::string>();
    if (useLogFile.empty()) {
        Util::Logger::setLogLevel(Util::LogLevel::Silent);
    } else {
        (void)freopen(useLogFile.c_str(), "w", stdout);
    }
}

void setAnalyzerOptions(cxxopts::ParseResult &result, WFC::AnalyzerOptions &options) {
    options = {
            static_cast<size_t>(result["pattern"].as<int>()),
            result["scale"].as<int>(),
            result["space"].as<int>(),
            result["rotate"].as<bool>(),
            result["flip"].as<bool>()
    };
}

void setBacktrackerOptions(cxxopts::ParseResult &result, WFC::BacktrackerOptions &options) {
    options = {
            static_cast<unsigned int>(result["depth"].as<int>()),
            static_cast<unsigned int>(result["max_iterations"].as<int>()),
            result["enable"].as<bool>()
    };
}

WFC::WFC createWFC(cxxopts::ParseResult &result, WFC::AnalyzerOptions &analyzerOptions, WFC::BacktrackerOptions &backtrackerOptions) {
    return WFC::WFC{result["input"].as<std::string>(),
               analyzerOptions,
               backtrackerOptions,
               static_cast<size_t>(result["width"].as<int>()),
               static_cast<size_t>(result["height"].as<int>())};
}

void setSavePaths(cxxopts::ParseResult &result, WFC::WFCSavePaths &savePaths) {
    savePaths = {
            result["patterns"].as<std::string>(),
            result["output"].as<std::string>(),
            result["failed"].as<std::string>(),
            result["iterations"].as<std::string>(),
            result["savePatterns"].as<bool>()
    };
}

int main(int argc, char *argv[]) {
    Util::CLI cli("WFC", "Wave Function Collapse");
    WFC::AnalyzerOptions analyzerOptions{};
    WFC::BacktrackerOptions backtrackerOptions{};
    WFC::WFCSavePaths savePaths{};
    cli.parseOptions(argc, argv);
    cxxopts::Options options = cli.getOptions();
    cxxopts::ParseResult result = cli.getResult();

    if (checkIfHelp(result, options)) {
        return 0;
    }

    setVerbose(result);
    setLogFile(result);
    setAnalyzerOptions(result, analyzerOptions);
    setBacktrackerOptions(result, backtrackerOptions);

    auto wfc = createWFC(result, analyzerOptions, backtrackerOptions);
    setSavePaths(result, savePaths);
    wfc.setSavePaths(savePaths);

    wfc.prepareWFC();
    wfc.startWFC();
    wfc.saveOutput();

    return 0;
}
