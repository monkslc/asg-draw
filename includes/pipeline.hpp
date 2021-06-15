#include "sviggy.hpp"

class Pipeline {
    public:
    DynamicArrayEx<Vec2Many, LinearAllocatorPool> bins;
    DynamicArrayEx<TagId, LinearAllocatorPool> tags;
    Pipeline() {};

    void Run(Document* input_doc, DXState* dx, LinearAllocatorPool* allocator);
    void RunBinPacking(Document* input_doc, DXState* dx, LinearAllocatorPool* allocator);
    void RunFilter(Document* input_doc, DXState* dx, LinearAllocatorPool* allocator);
};

struct CollectionBounds {
    DynamicArrayEx<RectNamed, LinearAllocatorPool> array;
    HashMapEx<size_t, Rect, LinearAllocatorPool> map;
};
CollectionBounds GetCollectionBounds(
    Document *doc,
    LinearAllocatorPool *allocator
);