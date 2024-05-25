//
// Created by Jakub on 25.05.2024.
//

#ifndef WFC_CLI_H
#define WFC_CLI_H


#include "../cxxopts.hpp"

namespace Util {
    class CLI {
    public:
        CLI(const std::string &name, const std::string &description);

        void parseOptions(int argc, char *argv[]);

        cxxopts::ParseResult &getResult();

        cxxopts::Options &getOptions();

    private:
        void initOptions();

    private:
        cxxopts::Options options;
        cxxopts::ParseResult result;
    };
}

#endif //WFC_CLI_H
