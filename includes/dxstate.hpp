#ifndef DXSTATE_H
#define DXSTATE_H

#include "sviggy.hpp"

// TODO: SafeRelease is actually a terible idea because it hides pointers being null that shouldn't be
// We should stop using it at some point
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