cmake_minimum_required(VERSION 3.22)
project(WFC)

set(CMAKE_CXX_STANDARD 17)

# Find the PNG and X11 packages
find_package(PNG REQUIRED)
find_package(X11 REQUIRED)

# Add executable and link against libpng and X11
add_executable(WFC main.cpp
        wfc/WFC.cpp
        wfc/WFC.h
        utility/Logger.cpp
        utility/Logger.h
        utility/Timer.cpp
        utility/Timer.h
        wfc/Analyzer.cpp
        wfc/Analyzer.h
        utility/Point.cpp
        utility/Point.h
        wfc/Backtracker.cpp
        wfc/Backtracker.h
        utility/CLI.cpp
        utility/CLI.h
        utility/FileUtil.cpp
        utility/FileUtil.h)

target_link_libraries(WFC PRIVATE PNG::PNG X11::X11)

# Add optimized executable
add_executable(WFC_optimized main.cpp
        wfc/WFC.cpp
        wfc/WFC.h
        utility/Logger.cpp
        utility/Logger.h
        utility/Timer.cpp
        utility/Timer.h
        wfc/Analyzer.cpp
        wfc/Analyzer.h
        utility/Point.cpp
        utility/Point.h
        wfc/Backtracker.cpp
        wfc/Backtracker.h
        utility/CLI.cpp
        utility/CLI.h
        utility/FileUtil.cpp
        utility/FileUtil.h)

target_link_libraries(WFC_optimized PRIVATE PNG::PNG X11::X11)

#set optimization flags for the optimized target
target_compile_options(WFC_optimized PRIVATE -O3)