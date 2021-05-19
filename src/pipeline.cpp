#include "ds.hpp"
#include "sviggy.hpp"

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