#include <d2d1.h>

#include "sviggy.hpp"

template <typename T>
D2D1_RECT_F ShapeBound(T* shape) {
    D2D1_RECT_F bound;
    HRESULT hr = shape->geometry->GetBounds(shape->TransformMatrix(), &bound);
    ExitOnFailure(hr);
    return bound;
}

template <typename T>
D2D1_RECT_F ShapeOriginalBound(T* shape) {
    D2D1_RECT_F bound;
    shape->geometry->GetBounds(D2D1::Matrix3x2F::Identity(), &bound);
    return bound;
}

template <typename T>
Vec2 ShapeCenter(T *shape) {
    D2D1_RECT_F bound = shape->Bound();
    float x = (bound.left + bound.right) / 2.0f;
    float y = (bound.top + bound.bottom) / 2.0f;

    return Vec2(x, y);
}

template <typename T>
Vec2 ShapeOriginalCenter(T *shape) {
    D2D1_RECT_F bound = shape->OriginalBound();
    float x = (bound.left + bound.right) / 2.0f;
    float y = (bound.top + bound.bottom) / 2.0f;

    return Vec2(x, y);
}

template <typename T>
D2D1::Matrix3x2F ShapeTransformMatrix(T *shape) {
    return shape->transform.Matrix(shape->OriginalCenter());
}