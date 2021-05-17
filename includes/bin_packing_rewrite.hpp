#ifndef BIN_PACKING_H
#define BIN_PACKING_H

#include "ds.hpp"
#include "sviggy.hpp"

// LeftX, RightX, etc methods, maybe throw a rotation on it
class Rect {
    public:
    Vec2 pos, size;
    Rect(Vec2 pos, Vec2 size);
};

class Bin {
    public:
    Vec2 size;
    DynamicArrayEx<Vec2Named, LinearAllocatorPool> rects;
    Bin(Vec2 size, LinearAllocatorPool *allocator);
};

DynamicArrayEx<Bin, LinearAllocatorPool> PackBins(
        DynamicArrayEx<Vec2Many, LinearAllocatorPool>* available_bins,
        DynamicArrayEx<Vec2Named, LinearAllocatorPool> *rects,
        LinearAllocatorPool *allocator
);

// Returns Null if none of the available areas work
Rect* FindNextAvailableArea(
    DynamicArrayEx<Bin,      LinearAllocatorPool>* bins,
    DynamicArrayEx<size_t,   LinearAllocatorPool>* used_bins_count,
    DynamicArrayEx<Vec2Many, LinearAllocatorPool>* available_bins,
    DynamicArrayEx<DynamicArrayEx<Rect, LinearAllocatorPool>, LinearAllocatorPool>* available_areas,
    Vec2 size,
    LinearAllocatorPool *allocator
);

#endif