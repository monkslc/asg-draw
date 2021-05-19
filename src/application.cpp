#include <d2d1.h>
#include <unordered_map>

#include "bin_packing.hpp"
#include "ds.hpp"
#include "pipeline.hpp"
#include "sviggy.hpp"

Application::Application() : documents(DynamicArray<Document>(1)), active_doc(0) {
    this->documents.Push(Document());
};

void Application::Free() {
    this->documents.FreeAll();
}

Document* Application::ActiveDoc() {
    return this->documents.GetPtr(this->active_doc);
}

View* Application::ActiveView() {
    return &this->documents.GetPtr(this->active_doc)->view;
}

void Application::ActivateDoc(size_t index) {
    if (this->documents.Length() <= index) {
        auto i = documents.Length();
        while (i <= index) {
            this->documents.Push(Document());
            i++;
        }
    }

    this->active_doc = index;
}

Document::Document() : texts(DynamicArray<Text>(10)), paths(DynamicArray<Path>(10)), active_shapes(DynamicArray<ActiveShape>(5)) {};

void Document::Free() {
    this->texts.FreeAll();
    this->paths.FreeAll();
    this->active_shapes.Free();
}

void Document::AddNewPath(Path p) {
    p.collection = this->NextCollection();
    this->paths.Push(p);
}

size_t Document::NextCollection() {
    size_t next = this->next_collection;
    this->next_collection++;
    return next;
}

bool EntirelyContains(D2D1_RECT_F* outer, D2D1_RECT_F* inner) {
    return
        outer->left   <= inner->left  &&
        outer->top    <= inner->top   &&
        outer->right  >= inner->right &&
        outer->bottom >= inner->bottom;
}

void Document::SelectShapes(Vec2 mousedown, Vec2 mouseup) {
    this->active_shapes.Clear();

    float minx = std::min<float>(mousedown.x, mouseup.x);
    float miny = std::min<float>(mousedown.y, mouseup.y);
    float maxx = std::max<float>(mousedown.x, mouseup.x);
    float maxy = std::max<float>(mousedown.y, mouseup.y);

    Vec2 start = Vec2(minx, miny);
    Vec2 end   = Vec2(maxx, maxy);

    D2D1_RECT_F selection = D2D1::RectF(start.x, start.y, end.x, end.y);
    for (auto i=0; i<this->paths.Length(); i++) {
       Path *path = this->paths.GetPtr(i);
       D2D1_RECT_F bound = path->Bound();

       if (EntirelyContains(&selection, &bound)) {
           this->active_shapes.Push(ActiveShape(ShapeType::Path, i));
       }
    }
}

void Document::SelectShape(Vec2 screen_pos) {
    this->active_shapes.Clear();

    D2D1_POINT_2F point = screen_pos.D2Point();

    D2D1::Matrix3x2F doc_to_screen = this->view.DocumentToScreenMat();

    for (auto i=0; i<this->paths.Length(); i++) {
        Path *path = this->paths.GetPtr(i);

        BOOL contains_point;
        path->geometry->StrokeContainsPoint(point, kHairline, NULL, path->TransformMatrix() * doc_to_screen, &contains_point);

        if (contains_point) {
            this->active_shapes.Push(ActiveShape(ShapeType::Path, i));
            return;
        }
    }
}

void Document::TranslateView(Vec2 amount) {
    this->view.start += amount;
}

void Document::ScrollZoom(bool in) {
    this->view.ScrollZoom(in);
}

Vec2 Document::MousePos() {
   return  this->view.MousePos();
}

void Document::RunPipeline(Document *doc) {
    size_t memory_estimation = this->paths.Length() * 100;
    LinearAllocatorPool allocator = LinearAllocatorPool(memory_estimation);
    CollectionMap collections = this->Collections(&allocator);

    // Bins will eventually come as input
    auto bins = DynamicArrayEx<Vec2Many, LinearAllocatorPool>(1, &allocator);
    bins.Push(Vec2Many(Vec2(48, 24), kInfinity), &allocator);

    auto collection_bounds = GetCollectionBounds(this, &collections, &allocator);


    DynamicArrayEx<Bin, LinearAllocatorPool> packed_bins = PackBins(&bins, &collection_bounds, &allocator);

    // TODO: make a clone method on the dynamic array class instead of adding to the array inside the for loop
    DynamicArray<Path> new_paths = DynamicArray<Path>(this->paths.Length());

    Vec2 bin_offset = Vec2(0.0f, 0.0f);
    for (auto i=0; i<packed_bins.Length(); i++) {
        Bin* bin = packed_bins.GetPtr(i);
        for (auto j=0; j<bin->rects.Length(); j++) {
            Vec2Named packed_collection = bin->rects.Get(j);
            DynamicArrayEx<size_t, LinearAllocatorPool>* collection = collections.GetPtr(packed_collection.id);

            // TODO: right now this performs a linear search of the array. We can index the collections in a hashmap
            // for a performance improvement later if needed
            Rect collection_bound = FindCollectionBound(packed_collection.id, &collection_bounds);
            for (auto shape_idx=0; shape_idx<collection->Length(); shape_idx++) {
                size_t shape_id = collection->Get(shape_idx);

                // TODO: this is bad because we now have two references to the geometry so when one of the shapes
                // deletes its reference we will fail but we can come back to this later
                Path duplicated_shape   = this->paths.Get(shape_id);
                D2D1_RECT_F shape_bound = duplicated_shape.Bound();

                Vec2 shape_offset = Vec2(shape_bound.left - collection_bound.Left(), shape_bound.top - collection_bound.Top());
                Vec2 desired      = bin_offset + packed_collection.vec2 + shape_offset;
                duplicated_shape.SetPos(desired);

                new_paths.Push(duplicated_shape);
            }
        }

        bin_offset += bin->size;
    }

    // TODO: make sure we free all of the old paths first
    // Right now we can't because there's quite a bit duplicated geometry
    doc->paths = new_paths;

    allocator.FreeAllocator();
    printf("Ran Pipeline :)\n\n\n");
}

// TODO: I don't LOOOOOVE the idea of using a hashmap here. The collection ids are just numbers so we might get away
// with just using an array. On the other hand the collection id can go way above the number of paths in a document
// so that might be a little too large. Anyway come back and think about this more later
CollectionMap Document::Collections(LinearAllocatorPool *allocator) {
    size_t map_capacity_estimation = this->paths.Length() / 2;
    auto collections = HashMapEx<size_t, DynamicArrayEx<size_t, LinearAllocatorPool>, LinearAllocatorPool>(map_capacity_estimation, allocator);

    for (auto i=0; i<this->paths.Length(); i++) {
        Path *path = this->paths.GetPtr(i);

        DynamicArrayEx<size_t, LinearAllocatorPool>* collection = collections.GetPtr(path->collection);
        if (!collection) {
           collection = collections.Set(path->collection, DynamicArrayEx<size_t, LinearAllocatorPool>(5, allocator), allocator);
        }

        collection->Push(i, allocator);
    }

    return collections;
}

View::View() : start(Vec2(0.0f, 0.0f)), mouse_pos_screen(Vec2(0.0f, 0.0f)) {};
Vec2 View::GetDocumentPosition(Vec2 screen_pos) {
    return this->ScreenToDocumentMat().TransformPoint(screen_pos.D2Point());
}

Vec2 View::MousePos() {
    return this->GetDocumentPosition(this->mouse_pos_screen);
}

// Scroll zoom adjusts the scale while preservice the mouse position in the document
void View::ScrollZoom(bool in) {
    Vec2 original_mouse_pos = this->MousePos();

    float delta = in ? 1 + kScaleDelta : 1-kScaleDelta;
    this->scale *= delta;

    Vec2 mouse_after_scale = this->MousePos();
    Vec2 mouse_change = original_mouse_pos - mouse_after_scale;

    this->start += mouse_change;
}

D2D1::Matrix3x2F View::ScaleMatrix() {
    return D2D1::Matrix3x2F::Scale(
        this->scale * kPixelsPerInch,
        this->scale * kPixelsPerInch,
        D2D1_POINT_2F()
    );
}

D2D1::Matrix3x2F View::TranslationMatrix() {
    return D2D1::Matrix3x2F::Translation(-this->start.x, -this->start.y);
}

D2D1::Matrix3x2F View::DocumentToScreenMat() {
    D2D1::Matrix3x2F scale_matrix = this->ScaleMatrix();
    D2D1::Matrix3x2F translation_matrix = this->TranslationMatrix();

    return translation_matrix * scale_matrix;
}

D2D1::Matrix3x2F View::ScreenToDocumentMat() {
    D2D1::Matrix3x2F mat = this->DocumentToScreenMat();
    mat.Invert();
    return mat;
}