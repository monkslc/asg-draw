#include "sviggy.hpp"

enum class PipelineActionType {
    Filter,
    Layout,
};

union PipelineActionValue {
    DynamicArrayEx<TagId, LinearAllocatorPool>    filter_tags;
    DynamicArrayEx<Vec2Many, LinearAllocatorPool> layout_bins;
};

class PipelineAction {
    public:
    PipelineActionType  type;
    PipelineActionValue value;
    PipelineAction(PipelineActionType type, PipelineActionValue value) : type(type), value(value) {};

    static PipelineAction Filter(DynamicArrayEx<TagId, LinearAllocatorPool> tags);
    static PipelineAction Layout(DynamicArrayEx<Vec2Many, LinearAllocatorPool> bins);
};

class PipelineActions {
    public:
    DynamicArrayEx<PipelineAction, LinearAllocatorPool> actions;
    PipelineActions() {};

    void Run(Document* input_doc, DXState* dx, LinearAllocatorPool* allocator);
};

void RunFilter(Document* input_doc, DXState* dx, LinearAllocatorPool* allocator, DynamicArrayEx<TagId, LinearAllocatorPool>* tags);
void RunLayout(Document* input_doc, DXState* dx, LinearAllocatorPool* allocator, DynamicArrayEx<Vec2Many, LinearAllocatorPool>* bins);

struct CollectionBounds {
    DynamicArrayEx<RectNamed, LinearAllocatorPool> array;
    HashMapEx<size_t, Rect, LinearAllocatorPool> map;
};

CollectionBounds GetCollectionBounds(
    Document *doc,
    LinearAllocatorPool *allocator
);