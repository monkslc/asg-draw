#include "bin_packing.hpp"
#include "ds.hpp"
#include "pipeline.hpp"
#include "shapes.hpp"
#include "sviggy.hpp"

void RunPipeline(Document* input_doc, Document* output_doc) {
    size_t memory_estimation = input_doc->transformed_geometries.Length() * 100;
    LinearAllocatorPool allocator = LinearAllocatorPool(memory_estimation);

    // Bins will eventually come as input
    auto bins = DynamicArrayEx<Vec2Many, LinearAllocatorPool>(1, &allocator);
    bins.Push(Vec2Many(Vec2(48, 24), kInfinity), &allocator);

    auto collection_bounds = GetCollectionBounds(input_doc, &allocator);

    DynamicArrayEx<Bin, LinearAllocatorPool> packed_bins = PackBins(&bins, &collection_bounds, &allocator);

    input_doc->pipeline_shapes.Clear();

    Vec2 bin_offset = Vec2(0.0f, 0.0f);
    for (auto i=0; i<packed_bins.Length(); i++) {
        Bin* bin = packed_bins.GetPtr(i);

        for (auto j=0; j<bin->rects.Length(); j++) {
            Vec2Named packed_collection = bin->rects.Get(j);
            DynamicArray<size_t>* collection = input_doc->reverse_collections_index.GetPtr(packed_collection.id);

            // TODO: right now this performs a linear search of the array. We can index the collections in a hashmap
            // for a performance improvement later if needed
            Rect collection_bound = FindCollectionBound(packed_collection.id, &collection_bounds);
            for (auto shape_idx=0; shape_idx<collection->Length(); shape_idx++) {
                size_t shape_id = collection->Get(shape_idx);

                // OK we want to duplicate the geometry and then set the pos, but we want to use the transformed geometry here
                size_t geometry_index              = *input_doc->transformed_geometries_index.GetPtr(shape_id);
                ID2D1TransformedGeometry* geometry = input_doc->transformed_geometries.Get(geometry_index);

                // Ok we have the geometry now we want to set its pos
                D2D1_RECT_F shape_bound;
                HRESULT hr = geometry->GetBounds(NULL, &shape_bound);
                ExitOnFailure(hr);

                Vec2 shape_offset = Vec2(shape_bound.left - collection_bound.Left(), shape_bound.top - collection_bound.Top());
                Vec2 desired      = bin_offset + packed_collection.vec2 + shape_offset;

                Transformation transformation = GetTranslationTo(desired, &shape_bound);

                input_doc->pipeline_shapes.Push(Shape(geometry, transformation));
            }
        }

        bin_offset += bin->size;
    }

    // TODO: make sure we free all of the old paths first
    // Right now we can't because there's quite a bit duplicated geometry
    // output_doc->paths = new_paths;

    allocator.FreeAllocator();
    printf("Ran Pipeline :)\n\n\n");
}

DynamicArrayEx<RectNamed, LinearAllocatorPool> GetCollectionBounds(Document *doc, LinearAllocatorPool *allocator) {
    auto rects = DynamicArrayEx<RectNamed, LinearAllocatorPool>(doc->reverse_collections_index.map.size, allocator);

    for (auto i=0; i<doc->reverse_collections_index.Capacity(); i++) {
        auto slot = doc->reverse_collections_index.Slot(i);

        for (auto j=0; j<slot->Length(); j++) {
            auto collection = slot->GetPtr(j);

            size_t shape_id = collection->value.Get(0);
            D2D1_RECT_F collection_bound = D2D1::RectF(kInfinity, kInfinity, kNegInfinity, kNegInfinity);

            for (auto k=0; k<collection->value.Length(); k++) {
                size_t shape_id               = collection->value.Get(k);
                size_t shape_index            = *doc->transformed_geometries_index.GetPtr(shape_id);
                ID2D1TransformedGeometry* geo = doc->transformed_geometries.Get(shape_index);

                D2D1_RECT_F shape_bound;
                HRESULT hr = geo->GetBounds(NULL, &shape_bound);
                ExitOnFailure(hr);

                Union(&collection_bound, &shape_bound);
            }

            float width  = collection_bound.right - collection_bound.left;
            float height = collection_bound.bottom   - collection_bound.top;
            Vec2 size = Vec2(width, height);
            Vec2 pos  = Vec2(collection_bound.left, collection_bound.top);

            rects.Push(RectNamed(Rect(pos, size), collection->key), allocator);
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