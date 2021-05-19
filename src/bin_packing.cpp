#include "bin_packing.hpp"
#include "ds.hpp"
#include "sviggy.hpp"

Bin::Bin(Vec2 size, LinearAllocatorPool *allocator) : size(size), rects(DynamicArrayEx<Vec2Named, LinearAllocatorPool>(20, allocator)) {};

DynamicArrayEx<Bin, LinearAllocatorPool> PackBins(
        DynamicArrayEx<Vec2Many,  LinearAllocatorPool>* available_bins,
        DynamicArrayEx<RectNamed, LinearAllocatorPool>* rects,
        LinearAllocatorPool *allocator
) {
    // Keeps track of which bins have been used. Each entry in this corresponds to an entry
    // at the same index in available_bins
    auto used_bins_count = DynamicArrayEx<size_t, LinearAllocatorPool>(available_bins->Length(), allocator);

    // Each entry in bins has an entry in available_areas at the same index that represents the available areas for the bin
    auto bins            = DynamicArrayEx<Bin, LinearAllocatorPool>(10, allocator);
    auto available_areas = DynamicArrayEx<DynamicArrayEx<Rect, LinearAllocatorPool>, LinearAllocatorPool>(10, allocator);

    for (auto i=0; i<rects->Length(); i++) {
        RectNamed* rect = rects->GetPtr(i);
        AvailableArea next_area = FindNextAvailableArea(&bins, &used_bins_count, available_bins, &available_areas, rect->rect.size, allocator);
        if (!next_area.area) {
            // TODO: we should return the fact that there was an error
            printf("We ran out of space :( returning early for now\n");
            return bins;
        }

        Place(&bins, &available_areas, rect, next_area, allocator);
    }

    // TODO: shrink bins

    return bins;
}

AvailableArea FindNextAvailableArea(
    DynamicArrayEx<Bin,      LinearAllocatorPool>* bins,
    DynamicArrayEx<size_t,   LinearAllocatorPool>* used_bins_count,
    DynamicArrayEx<Vec2Many, LinearAllocatorPool>* available_bins,
    DynamicArrayEx<DynamicArrayEx<Rect, LinearAllocatorPool>, LinearAllocatorPool>* available_areas,
    Vec2 size,
    LinearAllocatorPool *allocator
) {
    AvailableArea available_area = { };
    for (auto bin_id=0; bin_id<available_areas->Length(); bin_id++) {
        auto bin_areas = available_areas->GetPtr(bin_id);

        for (auto area_id=0; area_id<bin_areas->Length(); area_id++) {
            Rect* possible_fit = bin_areas->GetPtr(area_id);

            if (possible_fit->size.Fits(size)) {
                available_area.area = possible_fit;
                break;
            }
        }
    }

    if (!available_area.area) {
       for (auto i=0; i<available_bins->Length(); i++) {
           Vec2Many* possible_bin = available_bins->GetPtr(i);
           size_t*   used         = used_bins_count->GetPtr(i);

           if (*used >= possible_bin->quantity) continue;

           if (possible_bin->vec2.Fits(size)) {
               auto new_bin = Bin(possible_bin->vec2, allocator);
               bins->Push(new_bin, allocator);
               (*used)++;

               auto new_areas = DynamicArrayEx<Rect, LinearAllocatorPool>(10, allocator);
               available_areas->Push(new_areas, allocator);

               auto new_area = Rect(Vec2(0.0f, 0.0f), new_bin.size);
               available_areas->LastPtr()->Push(new_area, allocator);

               available_area.area   = available_areas->LastPtr()->LastPtr();
               available_area.bin_id = bins->Length() - 1;
           }
       }
    }

    return available_area;
}

void Place(
    DynamicArrayEx<Bin, LinearAllocatorPool>* bins,
    DynamicArrayEx<DynamicArrayEx<Rect, LinearAllocatorPool>, LinearAllocatorPool>* available_areas,
    RectNamed* rect,
    AvailableArea area,
    LinearAllocatorPool* allocator
) {
    Bin* bin       = bins->GetPtr(area.bin_id);
    auto bin_areas = available_areas->GetPtr(area.bin_id);


    Vec2Named packed_collection = Vec2Named(area.area->pos, rect->id);
    bin->rects.Push(packed_collection, allocator);

    RectPair new_areas = SplitVertical(area.area, rect->rect.size);

    // TODO: make sure the available areas stay in a sorted order
    *area.area = new_areas.a;
    bin_areas->Push(new_areas.b, allocator);
}

RectPair SplitVertical(Rect *rect, Vec2 size) {
    float left = rect->pos.x;
    float top  = rect->pos.y;

    float inner_right = left + size.x;
    float inner_bot   = top  + size.y;

    float outer_right = left + rect->size.x;
    float outer_bot   = top  + rect->size.y;

    Vec2 r1_start = Vec2(left, inner_bot);
    Vec2 r1_size  = Vec2(inner_right - left, outer_bot - inner_bot);
    Rect r1       = Rect(r1_start, r1_size);

    Vec2 r2_start = Vec2(inner_right, top);
    Vec2 r2_size  = Vec2(outer_right - inner_right, outer_bot - top);
    Rect r2       = Rect(r2_start, r2_size);

    return RectPair { r1, r2 };
}