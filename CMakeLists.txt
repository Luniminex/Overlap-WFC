cmake_minimum_required(VERSION 3.27)
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
        utility/Logger.h
        utility/Timer.cpp
        utility/Timer.h
        utility/Timer.h)
target_link_libraries(WFC PRIVATE PNG::PNG X11::X11)
