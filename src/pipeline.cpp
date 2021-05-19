#include "bin_packing.hpp"
#include "ds.hpp"
#include "pipeline.hpp"
#include "sviggy.hpp"

void RunPipeline(Document *input_doc, Document *output_doc) {
    size_t memory_estimation = input_doc->paths.Length() * 100;
    LinearAllocatorPool allocator = LinearAllocatorPool(memory_estimation);
    CollectionMap collections = input_doc->Collections(&allocator);

    // Bins will eventually come as input
    auto bins = DynamicArrayEx<Vec2Many, LinearAllocatorPool>(1, &allocator);
    bins.Push(Vec2Many(Vec2(48, 24), kInfinity), &allocator);

    auto collection_bounds = GetCollectionBounds(input_doc, &collections, &allocator);


    DynamicArrayEx<Bin, LinearAllocatorPool> packed_bins = PackBins(&bins, &collection_bounds, &allocator);

    // TODO: make a clone method on the dynamic array class instead of adding to the array inside the for loop
    DynamicArray<Path> new_paths = DynamicArray<Path>(input_doc->paths.Length());

    Vec2 bin_offset = Vec2(0.0f, 0.0f);
    for (auto i=0; i<packed_bins.Length(); i++) {
        Bin* bin = packed_bins.GetPtr(i);
        for (auto j=0; j<bin->rects.Length(); j++) {
            Vec2Named packed_collection = bin->rects.Get(j);
            DynamicArrayEx<size_t, LinearAllocatorPool>* collection = collections.GetPtr(packed_collection.id);

            // TODO: right now this performs a linear search of the array. We can index the collections in a hashmap
            // for a performance improvement later if needed
            Rect collection_bound = FindCollectionBound(packed_collection.id, &collection_bounds);
            for (auto shape_idx=0; shape_idx<collection->Length(); shape_idx++) {
                size_t shape_id = collection->Get(shape_idx);

                // TODO: this is bad because we now have two references to the geometry so when one of the shapes
                // deletes its reference we will fail but we can come back to this later
                Path duplicated_shape   = input_doc->paths.Get(shape_id);
                D2D1_RECT_F shape_bound = duplicated_shape.Bound();

                Vec2 shape_offset = Vec2(shape_bound.left - collection_bound.Left(), shape_bound.top - collection_bound.Top());
                Vec2 desired      = bin_offset + packed_collection.vec2 + shape_offset;
                duplicated_shape.SetPos(desired);

                new_paths.Push(duplicated_shape);
            }
        }

        bin_offset += bin->size;
    }

    // TODO: make sure we free all of the old paths first
    // Right now we can't because there's quite a bit duplicated geometry
    output_doc->paths = new_paths;

    allocator.FreeAllocator();
    printf("Ran Pipeline :)\n\n\n");

}

DynamicArrayEx<RectNamed, LinearAllocatorPool> GetCollectionBounds(Document *doc, CollectionMap *collections, LinearAllocatorPool *allocator) {
    auto rects = DynamicArrayEx<RectNamed, LinearAllocatorPool>(collections->Capacity(), allocator);
    for (auto i=0; i<collections->Capacity(); i++) {
        auto slot = collections->Slot(i);
        if (slot->Length() == 0) continue;

        for (auto j=0; j<slot->Length(); j++) {
            auto *entry          = slot->GetPtr(j);
            size_t collection_id = entry->key;
            auto* collection     = &entry->value;

            size_t shape_id   = collection->Get(0);
            D2D1_RECT_F bound = doc->paths.GetPtr(shape_id)->Bound();
            for (auto k=1; k<collection->Length(); k++) {
                shape_id = collection->Get(k);

                D2D1_RECT_F new_shape_bound = doc->paths.GetPtr(shape_id)->Bound();
                Union(&bound, &new_shape_bound);
            }

            float width  = bound.right - bound.left;
            float height = bound.bottom - bound.top;
            Vec2 size    = Vec2(width, height);
            Vec2 pos     = Vec2(bound.left, bound.top);

            rects.Push(RectNamed(Rect(pos, size), collection_id), allocator);
        }
    }

    return rects;
}

Rect FindCollectionBound(size_t needle, DynamicArrayEx<RectNamed, LinearAllocatorPool> *haystack) {
    for (auto i=0; i<haystack->Length(); i++) {
        RectNamed* potential = haystack->GetPtr(i);
        if (potential->id == needle) {
            return potential->rect;
        }
    }

    // TODO: We shouldn't get to this point but if we do we should probably assert(false);
    return Rect(Vec2(0.0f, 0.0f), Vec2(0.0f, 0.0f));
}