//
// Created by Jakub on 19.04.2024.
//

#include "Point.h"

Point::Point(int x, int y) : x(x), y(y) {}

bool Point::operator==(const Point &other) const {
    return x == other.x && y == other.y;
}

Point Point::operator+(const Point &other) const {
    return {x + other.x, y + other.y};
}

Point Point::operator-(const Point &other) const {
    return {x - other.x, y - other.y};
}

bool Point::operator<(const Point &other) const {
    if (x == other.x) {
        return y < other.y;
    }
    return x < other.x;
}

std::size_t PointHash::operator()(const Point &p) const {
    return std::hash<int>()(p.x) ^ std::hash<int>()(p.y);
}
