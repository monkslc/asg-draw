#include "sviggy.hpp"

class Pipeline {
    public:
    DynamicArrayEx<Vec2Many, LinearAllocatorPool> bins;
    DynamicArrayEx<StringEx<LinearAllocatorPool>, LinearAllocatorPool> tags;
    Pipeline() {};

    void Run(Document* input_doc, LinearAllocatorPool* allocator);
};

struct CollectionBounds {
    DynamicArrayEx<RectNamed, LinearAllocatorPool> array;
    HashMapEx<size_t, Rect, LinearAllocatorPool> map;
};
CollectionBounds GetCollectionBounds(
    Document *doc,
    LinearAllocatorPool *allocator
);