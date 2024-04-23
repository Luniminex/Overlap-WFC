//
// Created by Jakub on 19.04.2024.
//

#ifndef WFC_POINT_H
#define WFC_POINT_H

#include <cstddef>
#include <functional>

struct Point {
    int x;
    int y;

    Point(int x, int y);
    bool operator==(const Point &other) const;
    Point operator+(const Point &other) const;
    Point operator-(const Point &other) const;
    bool operator<(const Point &other) const;
};

struct PointHash {
    std::size_t operator()(const Point &p) const;
};
#endif //WFC_POINT_H
