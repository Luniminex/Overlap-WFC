//
// Created by Jakub on 25.05.2024.
//

#include "CLI.h"

Util::CLI::CLI(const std::string& name, const std::string& description) : options(name, description){
    initOptions();
}

void Util::CLI::initOptions() {
    options.add_options()
            ("i,input", "Input image path", cxxopts::value<std::string>()->default_value("../resources/Cave.png"))
            ("p,pattern", "Pattern size", cxxopts::value<int>()->default_value("3"))
            ("s,scale", "Preview image scale", cxxopts::value<int>()->default_value("64"))
            ("b,space", "Space between", cxxopts::value<int>()->default_value("16"))
            //these would be ideally set to true as default, but since cxxopts doesn't support i.e. -r false, we have
            //to do it with the absence of the flag signalling false. it does not convert default true to false with non absent flag
            ("r,rotate", "Enable rotation", cxxopts::value<bool>()->default_value("false"))
            ("f,flip", "Enable flip", cxxopts::value<bool>()->default_value("false"))
            ("d,depth", "Backtracker depth", cxxopts::value<int>()->default_value("50"))
            ("m,max_iterations", "Backtracker max iterations", cxxopts::value<int>()->default_value("3"))
            ("e,enable", "Enable backtracker", cxxopts::value<bool>()->default_value("false"))
            ("w,width", "Output width", cxxopts::value<int>()->default_value("16"))
            ("h,height", "Output height", cxxopts::value<int>()->default_value("16"))
            ("l,log", "Specify the file where to write logs, leave empty for no log file",
             cxxopts::value<std::string>()->default_value("../wfc.log"))
            ("v,verbose", "Verbose logging", cxxopts::value<bool>()->default_value("false"))
            ("o,output", "Output image directory", cxxopts::value<std::string>()->default_value("../outputs/solutions/"))
            ("a,iterations", "Iterations directory",
             cxxopts::value<std::string>()->default_value("../outputs/iterations/"))
            ("t,patterns", "Generated patterns directory",
             cxxopts::value<std::string>()->default_value("../outputs/patterns/"))
            ("c,failed", "Failed output image directory",
             cxxopts::value<std::string>()->default_value("../outputs/failed/"))
            ("y,savePatterns", "Save patterns, iterations and failed output images",
             cxxopts::value<bool>()->default_value("false"))
            ("help", "CLI for running WFC algorithm");
}

cxxopts::ParseResult &Util::CLI::getResult() {
    return result;
}

void Util::CLI::parseOptions(int argc, char **argv) {
    result = options.parse(argc, argv);
}

cxxopts::Options &Util::CLI::getOptions() {
    return options;
}
