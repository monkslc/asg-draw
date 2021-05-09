#ifndef D2STATE_H
#define D2STATE_H

#include "sviggy.hpp"

template <typename T>
void RenderShape(D2State *d2, T* shape) {
    ID2D1TransformedGeometry *geo;
    shape->transform.rotation += 1;
    HRESULT hr = d2->factory->CreateTransformedGeometry(
        shape->geometry,
        shape->TransformMatrix(),
        &geo
    );
    d2->renderTarget->DrawGeometry(geo, d2->blackBrush, kHairline);

    // TODO: release transformed geometry?
}

#endif