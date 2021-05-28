#include "bin_packing.hpp"
#include "ds.hpp"
#include "pipeline.hpp"
#include "shapes.hpp"
#include "sviggy.hpp"

#include <chrono>

void Pipeline::Run(Document* input_doc, LinearAllocatorPool* allocator) {
    auto begin = std::chrono::high_resolution_clock::now();

    auto collection_bounds = GetCollectionBounds(input_doc, allocator);

    auto e1 = std::chrono::high_resolution_clock::now();

    DynamicArrayEx<Bin, LinearAllocatorPool> packed_bins = PackBins(&this->bins, &collection_bounds.array, allocator);

    auto e2 = std::chrono::high_resolution_clock::now();

    input_doc->pipeline_shapes.Clear();

    Vec2 bin_offset = Vec2(0.0f, 0.0f);
    for (auto i=0; i<packed_bins.Length(); i++) {
        Bin* bin = packed_bins.GetPtr(i);

        for (auto j=0; j<bin->rects.Length(); j++) {
            Vec2Named packed_collection = bin->rects.Get(j);
            DynamicArray<size_t>* collection = input_doc->reverse_collections_index.GetPtr(packed_collection.id);

            Rect collection_bound = *collection_bounds.map.GetPtr(packed_collection.id);
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

    auto e3 = std::chrono::high_resolution_clock::now();
    auto el1 = std::chrono::duration_cast<std::chrono::nanoseconds>(e1 - begin);
    auto el2 = std::chrono::duration_cast<std::chrono::nanoseconds>(e2 - begin);
    auto el3 = std::chrono::duration_cast<std::chrono::nanoseconds>(e3 - begin);
    printf("Pipeline ran in %.3f seconds.\n", el1.count() * 1e-9);
    printf("Pipeline ran in %.3f seconds.\n", el2.count() * 1e-9);
    printf("Pipeline ran in %.3f seconds.\n", el3.count() * 1e-9);

    printf("Ran Pipeline :)\n\n\n");
}

CollectionBounds GetCollectionBounds(Document *doc, LinearAllocatorPool *allocator) {
    CollectionBounds bounds = {
        DynamicArrayEx<RectNamed, LinearAllocatorPool>(doc->reverse_collections_index.map.size, allocator),
        HashMapEx<size_t, Rect, LinearAllocatorPool>(doc->reverse_collections_index.map.size, allocator),
    };

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

            bounds.array.Push(RectNamed(Rect(pos, size), collection->key), allocator);
            bounds.map.Set(collection->key, Rect(pos, size), allocator);
        }
    }

    return bounds;
}