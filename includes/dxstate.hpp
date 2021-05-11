#ifndef DXSTATE_H
#define DXSTATE_H

#include "sviggy.hpp"

template <typename T>
HRESULT RenderShape(DXState *d2, T* shape) {
    HRESULT hr;

    ID2D1TransformedGeometry *geo;
    shape->transform.rotation += 1;
    hr = d2->factory->CreateTransformedGeometry(
        shape->geometry,
        shape->TransformMatrix(),
        &geo
    );
    RETURN_FAIL(hr);

    d2->renderTarget->DrawGeometry(geo, d2->blackBrush, kHairline);

    hr = geo->Release();
    return hr;
}

template <typename T>
HRESULT SafeRelease(T **h)
{
    HRESULT hr = S_OK;
    if (*h) {
        hr = (*h)->Release();
        *h = NULL;
    }

    return hr;
}

Vec2 ParseVec(char **iter);

#endif