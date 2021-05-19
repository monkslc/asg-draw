#include "sviggy.hpp"

DynamicArrayEx<RectNamed, LinearAllocatorPool> GetCollectionBounds(Document *doc, CollectionMap *collections, LinearAllocatorPool *allocator);

Rect FindCollectionBound(size_t needle, DynamicArrayEx<RectNamed, LinearAllocatorPool> *haystack);