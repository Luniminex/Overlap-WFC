//
// Created by Jakub on 19.04.2024.
//

#include "Point.h"

Util::Point::Point(int x, int y) : x(x), y(y) {}

bool Util::Point::operator==(const Point &other) const {
    return x == other.x && y == other.y;
}

Util::Point Util::Point::operator+(const Point &other) const {
    return Point(x + other.x, y + other.y);
}

bool Util::Point::operator!=(const Point &other) const {
    return !(*this == other);
}

Util::Point Util::Point::operator-(const Point &other) const {
    return Point(x - other.x, y - other.y);
}

bool Util::Point::operator<(const Point &other) const {
    return x < other.x || (x == other.x && y < other.y);
}

std::size_t Util::PointHash::operator()(const Point &p) const {
    std::hash<int> hash_fn;
    return hash_fn(p.x) ^ (hash_fn(p.y) << 1);
}
