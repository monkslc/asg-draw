#include "sviggy.hpp"

void RunPipeline(Document *input_doc, Document *output_doc);
DynamicArrayEx<RectNamed, LinearAllocatorPool> GetCollectionBounds(
    Document *doc,
    LinearAllocatorPool *allocator
);
Rect FindCollectionBound(size_t needle, DynamicArrayEx<RectNamed, LinearAllocatorPool> *haystack);