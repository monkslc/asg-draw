#include "sviggy.hpp"

Vec2::Vec2(float x, float y) : x(x), y(y) {};
Vec2::Vec2(D2D1_POINT_2F p) : x(p.x), y(p.y) {};

D2D1_POINT_2F Vec2::D2Point() {
    return D2D1::Point2F(x, y);
};

Vec2 Vec2::operator+(Vec2 &b) {
    return Vec2(this->x + b.x, this->y + b.y);
}

Vec2& Vec2::operator+=(Vec2 &b) {
    this->x += b.x;
    this->y += b.y;

    return *this;
}

Vec2 Vec2::operator-(Vec2 &b) {
    return Vec2(this->x - b.x, this->y - b.y);
}

Vec2& Vec2::operator-=(Vec2 &b) {
    this->x -= b.x;
    this->y -= b.y;

    return *this;
}