#include "sviggy.hpp"

Rect::Rect(float x, float y, float width, float height) : pos(Vec2(x, y)), size(Vec2(width, height)) {};
Rect::Rect(Vec2 pos, Vec2 size) : pos(pos), size(size) {};

float Rect::X() {
    return this->pos.x;
}

float Rect::Y() {
    return this->pos.y;
}

float Rect::Width() {
    return this->size.x;
}

float Rect::Height() {
    return this->size.y;
}

float Rect::Left() {
    return this->pos.x;
}

float Rect::Top() {
    return this->pos.y;
}

float Rect::Right() {
    return this->pos.x + this->size.x;
}

float Rect::Bottom() {
    return this->pos.y + this->size.y;
}

Text::Text(float x, float y, std::string text) : pos(Vec2(x, y)), text(text) {};
Text::Text(Vec2 pos, std::string text) : pos(pos), text(text) {};

float Text::X() {
    return this->pos.x;
}

float Text::Y() {
    return this->pos.y;
}

Line::Line(Vec2 start, Vec2 end) : start(start), end(end) {};
Line::Line(float x1, float y1, float x2, float y2) : start(Vec2(x1, y1)), end(Vec2(x2, y2)) {};

Poly::Poly() {};
Poly::Poly(std::vector<Vec2> points) : points(points) {};

Circle::Circle(Vec2 center, float radius) : center(center), radius(radius) {};
Circle::Circle(float x, float y, float radius) : center(Vec2(x, y)), radius(radius) {};

Path::Path(std::vector<float> commands) : commands(commands) {};