#include "bin_packing.hpp"
#include "ds.hpp"
#include "pipeline.hpp"
#include "shapes.hpp"
#include "sviggy.hpp"

#include <chrono>

void Pipeline::Run(Document* input_doc, DXState *dx, LinearAllocatorPool* allocator) {
    auto begin = std::chrono::high_resolution_clock::now();

    // Free the old memory and copy over a fresh copy of the paths
    // There's a performance gain to be had by resuing the memory instead
    // of blindly freeing but this works for now

    // TODO: replace free with a clear for pipeline
    // input_doc->pipeline_shapes.Free();
    input_doc->pipeline_shapes = input_doc->paths.Clone();
    input_doc->pipeline_shapes.RealizeAllHighFidelityGeometry(dx);
    auto e1 = std::chrono::high_resolution_clock::now();

    this->RunFilter(input_doc, dx, allocator);

    auto e2 = std::chrono::high_resolution_clock::now();

    this->RunBinPacking(input_doc, dx, allocator);

    input_doc->pipeline_shapes.RealizeAllHighFidelityGeometry(dx);

    auto e3 = std::chrono::high_resolution_clock::now();

    auto el1 = std::chrono::duration_cast<std::chrono::nanoseconds>(e1 - begin);
    auto el2 = std::chrono::duration_cast<std::chrono::nanoseconds>(e2 - begin);
    auto el3 = std::chrono::duration_cast<std::chrono::nanoseconds>(e3 - begin);
    printf("Pipeline ran in %.3f seconds.\n", el1.count() * 1e-9);
    printf("Pipeline ran in %.3f seconds.\n", el2.count() * 1e-9);
    printf("Pipeline ran in %.3f seconds.\n", el3.count() * 1e-9);

    printf("Ran Pipeline :)\n\n\n");
}

void Pipeline::RunFilter(Document* input_doc, DXState* dx, LinearAllocatorPool* allocator) {
    auto keep_shapes = HashMapEx<PathId, bool, LinearAllocatorPool>(input_doc->pipeline_shapes.Length() * 2, allocator);

    for (auto &entry : input_doc->pipeline_shapes.tags.reverse_tags) {
        TagId tag  = entry.key;
        if (this->tags.Find(tag)) {
            DynamicArray<PathId> shapes = entry.value;
            for (auto &id : shapes) {
                keep_shapes.Set(id, true, allocator);
            }
        }
    }

    for (int i=0; i<input_doc->pipeline_shapes.index.Capacity(); i++) {
        auto slot = input_doc->pipeline_shapes.index.Slot(i);
        for (int j=0; j<slot->Length(); j++) {
            auto entry = slot->Get(j);
            PathId id = entry.key;
            if(!keep_shapes.GetPtr(id)) {
                input_doc->pipeline_shapes.DeletePath(id);

                // Decrement j so we can process the element that was moved into the current slot in place of the deleted one
                j--;
            }
        }
    }
}

void Pipeline::RunBinPacking(Document* input_doc, DXState *dx, LinearAllocatorPool* allocator) {
    auto collection_bounds = GetCollectionBounds(input_doc, allocator);

    DynamicArrayEx<Bin, LinearAllocatorPool> packed_bins = PackBins(&this->bins, &collection_bounds.array, allocator);

    Vec2 bin_offset = Vec2(0.0f, 0.0f);
    for (auto i=0; i<packed_bins.Length(); i++) {
        Bin* bin = &packed_bins[i];

        for (auto j=0; j<bin->rects.Length(); j++) {
            Vec2Named packed_collection = bin->rects[j];
            DynamicArray<size_t>* collection = input_doc->pipeline_shapes.collections.reverse_collections_index.GetPtr(packed_collection.id);

            Rect collection_bound = *collection_bounds.map.GetPtr(packed_collection.id);
            for (auto shape_idx=0; shape_idx<collection->Length(); shape_idx++) {
                size_t shape_id = collection->Get(shape_idx);

                ID2D1TransformedGeometry* geometry = *input_doc->pipeline_shapes.GetTransformedGeometry(shape_id);

                Rect shape_bound = GetBounds(geometry);

                Vec2 shape_offset = Vec2(shape_bound.Left() - collection_bound.Left(), shape_bound.Top() - collection_bound.Top());
                Vec2 desired      = bin_offset + packed_collection.vec2 + shape_offset;

                // Right now we're not really setup to provide a transformation on top of a current transformation
                // This could be fixed by just moving the transformation to a matrix. Since we only need to worry
                // about the translation here, just add the translation from the new transformation to the original
                Transformation transformation = GetTranslationTo(desired, &shape_bound);
                ShapeData *data = input_doc->pipeline_shapes.GetShapeData(shape_id);
                data->transform.translation += transformation.translation;
            }
        }

        bin_offset += bin->size;
    }
}

CollectionBounds GetCollectionBounds(Document *doc, LinearAllocatorPool *allocator) {
    size_t collection_count = doc->pipeline_shapes.collections.reverse_collections_index.Length();
    CollectionBounds bounds = {
        DynamicArrayEx<RectNamed, LinearAllocatorPool>(collection_count, allocator),
        HashMapEx<size_t, Rect, LinearAllocatorPool>(collection_count, allocator),
    };

    for (auto &entry : doc->pipeline_shapes.collections.reverse_collections_index) {
        CollectionId collection     = entry.key;
        DynamicArray<PathId> shapes = entry.value;

        size_t shape_id       = shapes[0];
        Rect collection_bound = doc->pipeline_shapes.GetBounds(shape_id);

        for (auto k=1; k<shapes.Length(); k++) {
            shape_id = shapes[k];

            ID2D1TransformedGeometry* geo = *doc->pipeline_shapes.GetTransformedGeometry(shape_id);

            Rect shape_bound = GetBounds(geo);

            collection_bound = collection_bound.Union(&shape_bound);
        }

        bounds.array.Push(RectNamed(collection_bound, collection), allocator);
        bounds.map.Set(collection, collection_bound, allocator);
    }

    return bounds;
}
