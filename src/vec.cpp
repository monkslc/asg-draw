#include <d2d1.h>
#include <limits>

#include "sviggy.hpp"

Vec2::Vec2(float x, float y) : x(x), y(y) {};
Vec2::Vec2(D2D1_POINT_2F p) : x(p.x), y(p.y) {};

Vec2 Vec2::Min() {
    return Vec2(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
}

Vec2 Vec2::Max() {
    return Vec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
}

D2D1_POINT_2F Vec2::D2Point() {
    return D2D1::Point2F(x, y);
};

D2D1_SIZE_F Vec2::Size() {
    return D2D1::SizeF(this->x, this->y);
}

bool Vec2::Fits(Vec2 other) {
    // TODO: better floating point comaprison
    return this->x >= other.x && this->y >= other.y;
}

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

Vec2 Vec2::operator/(float b) {
    return Vec2(this->x / b, this->y / b);
}

Vec2 Vec2::operator-() {
    return Vec2(-this->x, -this->y);
}

Vec2Many::Vec2Many(Vec2 vec2, float quantity) : vec2(vec2), quantity(quantity) {};

Vec2Named::Vec2Named(Vec2 vec2, size_t id) : vec2(vec2), id(id) {};