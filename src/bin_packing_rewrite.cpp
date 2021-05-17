#include "bin_packing.hpp"
#include "ds.hpp"
#include "sviggy.hpp"

Rect::Rect(Vec2 pos, Vec2 size) : pos(pos), size(size) {};

Bin::Bin(Vec2 size, LinearAllocatorPool *allocator) : size(size), rects(DynamicArrayEx<Vec2Named, LinearAllocatorPool>(20, allocator)) {};

DynamicArrayEx<Bin, LinearAllocatorPool> PackBins(
        DynamicArrayEx<Vec2Many,  LinearAllocatorPool>* available_bins,
        DynamicArrayEx<Vec2Named, LinearAllocatorPool>* rects,
        LinearAllocatorPool *allocator
) {
    // Each entry in bins has an entry at the same index in used_bins_count that represents the number of times the bin was used and
    // an entry in available_areas at the same index that lists all of the available packing space left for that bin
    auto bins            = DynamicArrayEx<Bin, LinearAllocatorPool>(10, allocator);
    auto used_bins_count = DynamicArrayEx<size_t, LinearAllocatorPool>(bins.Length(), allocator);
    auto available_areas = DynamicArrayEx<DynamicArrayEx<Rect, LinearAllocatorPool>, LinearAllocatorPool>(10, allocator);

    for (auto i=0; i<rects->Length(); i++) {
        Vec2Named* rect = rects->GetPtr(i);

    }

    return bins;
}

Rect* FindNextAvailableArea(
    DynamicArrayEx<Bin,      LinearAllocatorPool>* bins,
    DynamicArrayEx<size_t,   LinearAllocatorPool>* used_bins_count,
    DynamicArrayEx<Vec2Many, LinearAllocatorPool>* available_bins,
    DynamicArrayEx<DynamicArrayEx<Rect, LinearAllocatorPool>, LinearAllocatorPool>* available_areas,
    Vec2 size,
    LinearAllocatorPool *allocator
) {
    Rect *available_area = NULL;
    for (auto i=0; i<available_areas->Length(); i++) {
        auto bin_i_areas = available_areas->GetPtr(i);

        for (auto j=0; j<bin_i_areas->Length(); j++) {
            Rect* possible_fit = bin_i_areas->GetPtr(i);

            if (possible_fit->size.Fits(size)) {
                available_area = possible_fit;
                break;
            }
        }
    }

    if (!available_area) {
       for (auto i=0; i<available_bins->Length(); i++) {
           size_t used = used_bins_count->Get(i);

           Vec2Many *possible_bin = available_bins->GetPtr(i);
           if (used >= possible_bin->quantity) continue;

           if (possible_bin->vec2.Fits(size)) {
               bins->Push(Bin(possible_bin->vec2, allocator), allocator);
               (*used_bins_count->GetPtr(i))++;

               // Working here. We found a bin that fits but we need to add it to the available areas and set availble_area* to it
               //available_areas->Push()
               //available_areas.GetPtr(i)->Push(Rect(Vec2(0.0f, 0.0f), Vec2(possible_bin->vec2)))
           }
       }
    }

    return available_area;
}