#ifndef BIN_PACKING_H
#define BIN_PACKING_H

#include "ds.hpp"
#include "sviggy.hpp"

class Bin {
    public:
    Vec2 size;
    DynamicArrayEx<Vec2Named, LinearAllocatorPool> rects;
    Bin(Vec2 size, LinearAllocatorPool *allocator);
};

DynamicArrayEx<Bin, LinearAllocatorPool> PackBins(
        DynamicArrayEx<Vec2Many, LinearAllocatorPool>* available_bins,
        DynamicArrayEx<RectNamed, LinearAllocatorPool> *rects,
        LinearAllocatorPool *allocator
);

struct AvailableArea {
    Rect* area;
    size_t bin_id;
};

// Returns NULL for AvailableArea.aera if none of the available areas can fit the shape
AvailableArea FindNextAvailableArea(
    DynamicArrayEx<Bin,      LinearAllocatorPool>* bins,
    DynamicArrayEx<size_t,   LinearAllocatorPool>* used_bins_count,
    DynamicArrayEx<Vec2Many, LinearAllocatorPool>* available_bins,
    DynamicArrayEx<DynamicArrayEx<Rect, LinearAllocatorPool>, LinearAllocatorPool>* available_areas,
    Vec2 size,
    LinearAllocatorPool *allocator
);

void Place(
    DynamicArrayEx<Bin, LinearAllocatorPool>* bins,
    DynamicArrayEx<DynamicArrayEx<Rect, LinearAllocatorPool>, LinearAllocatorPool>* available_areas,
    RectNamed* rect,
    AvailableArea area,
    LinearAllocatorPool* allocator
);

struct RectPair {
    Rect a, b;
};

RectPair SplitVertical(Rect *rect, Vec2 size);

#endif