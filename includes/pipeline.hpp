#include "sviggy.hpp"

void RunPipeline(Document *input_doc, Document *output_doc);

struct CollectionBounds {
    DynamicArrayEx<RectNamed, LinearAllocatorPool> array;
    HashMapEx<size_t, Rect, LinearAllocatorPool> map;
};
CollectionBounds GetCollectionBounds(
    Document *doc,
    LinearAllocatorPool *allocator
);

Rect FindCollectionBound(size_t needle, DynamicArrayEx<RectNamed, LinearAllocatorPool> *haystack);