#include <algorithm>
#include <d2d1.h>
#include <unordered_map>

#include "bin_packing.hpp"
#include "ds.hpp"
#include "pipeline.hpp"
#include "sviggy.hpp"

#include <chrono>

constexpr size_t kDefaultEstimatedShapes = 1000;

Application::Application() : documents(DynamicArray<Document>(1)), active_doc(0) {
    this->documents.Push(Document(kDefaultEstimatedShapes));
};

void Application::Free() {
    this->documents.FreeAll();
}

Document* Application::ActiveDoc() {
    return &this->documents[this->active_doc];
}

View* Application::ActiveView() {
    return &this->documents.GetPtr(this->active_doc)->view;
}

void Application::ActivateDoc(size_t index) {
    if (this->documents.Length() <= index) {
        auto i = documents.Length();
        while (i <= index) {
            this->documents.Push(Document(kDefaultEstimatedShapes));
            i++;
        }
    }

    this->active_doc = index;
}

Document::Document(size_t estimated_shapes) :
    paths(Paths(estimated_shapes)),

    texts(DynamicArray<Text>(10)),

    active_shapes(DynamicArray<ActiveShape>(5)),

    pipeline_shapes(Paths(estimated_shapes)) {};

void Document::Free() {
    this->texts.FreeAll();

    this->paths.FreeAndReleaseResources();
    this->pipeline_shapes.Free();

    this->active_shapes.Free();
}

void Document::AddNewPath(ShapeData p) {
    PathId id = this->paths.AddPath(p);

    this->paths.collections.CreateCollectionForShape(id);
}

void Document::AssignTag(PathId id, String tag) {
    TagId tag_id = this->tag_god.GetTagId(tag);

    this->paths.tags.AssignTag(id, tag_id);
}

void Document::SelectShapes(Vec2 mousedown, Vec2 mouseup) {
    this->active_shapes.Clear();

    float minx = std::min<float>(mousedown.x, mouseup.x);
    float miny = std::min<float>(mousedown.y, mouseup.y);
    float maxx = std::max<float>(mousedown.x, mouseup.x);
    float maxy = std::max<float>(mousedown.y, mouseup.y);

    Vec2 start = Vec2(minx, miny);
    Vec2 end   = Vec2(maxx, maxy);

    Vec2 size = end - start;

    Rect selection = Rect(start, size);

    for (auto i=0; i<this->paths.Length(); i++) {
        Rect shape_bound = GetBounds(this->paths.transformed_geometries[i]);
        if (selection.Contains(&shape_bound)) {
            PathId path_id = this->paths.reverse_index[i];
            this->active_shapes.Push(ActiveShape(path_id));
        }
    }
}

void Document::SelectShape(Vec2 screen_pos) {
    this->active_shapes.Clear();

    D2D1_POINT_2F point = screen_pos.D2Point();

    D2D1::Matrix3x2F doc_to_screen = this->view.DocumentToScreenMat();

    for (auto i=0; i<this->paths.Length(); i++) {
        ID2D1TransformedGeometry* geo = this->paths.transformed_geometries[i];
        BOOL contains_point;

        HRESULT hr = geo->StrokeContainsPoint(point, kHairline, NULL, &doc_to_screen, &contains_point);
        ExitOnFailure(hr);

        if (contains_point) {
            size_t path_id = this->paths.reverse_index[i];
            this->active_shapes.Push(ActiveShape(path_id));
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
   return this->view.MousePos();
}

void Document::TogglePipelineView() {
    this->view.show_pipeline = !this->view.show_pipeline;
}

void Document::CollectActiveShapes() {
    size_t collection = this->paths.collections.NextId();
    for (auto& shape : this->active_shapes) {
        this->paths.collections.SetCollection(shape.id, collection);
    }
}

void Document::AutoCollect() {
    auto begin = std::chrono::high_resolution_clock::now();

    size_t memory_estimation = this->paths.Length() * 2 * sizeof(RectNamed);
    LinearAllocatorPool allocator = LinearAllocatorPool(memory_estimation);

    auto shape_bounds = DynamicArrayEx<RectNamed, LinearAllocatorPool>(this->paths.Length(), &allocator);
    for (auto i=0; i<this->paths.Length(); i++) {
        size_t shape_id               = this->paths.reverse_index[i];
        ID2D1TransformedGeometry* geo = this->paths.transformed_geometries[i];

        Rect bounds = GetBounds(geo);
        shape_bounds.Push(RectNamed(bounds, shape_id), &allocator);
    }

    std::sort(shape_bounds.Data(), shape_bounds.End(), SortRectPositionXY<RectNamed>);

    auto new_collections = DynamicArrayEx<RectNamed, LinearAllocatorPool>(this->paths.Length(), &allocator);
    auto search_from = 0;
    for (auto &shape : shape_bounds) {
        bool found_fit = false;
        for (auto j=search_from; j<new_collections.Length(); j++) {
            RectNamed* collection = &new_collections[j];
            if (collection->rect.Contains(&shape.rect)) {
                this->paths.collections.SetCollection(shape.id, collection->id);
                found_fit = true;
                break;
            }

            // Since the shapes are sorted, the collection will not be able to hold any of the
            // the shapes that follow if shape.left is greater than the collection right so we
            // advance search_from
            if (shape.Left() > collection->Right()) {
                search_from = j;
            }
        }

        if (!found_fit) {
            size_t next_collection = this->paths.collections.NextId();
            this->paths.collections.SetCollection(shape.id, next_collection);
            new_collections.Push(RectNamed(shape.rect, next_collection), &allocator);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
    printf("Auto collect ran in %.3f seconds.\n", elapsed.count() * 1e-9);
}

View::View() : start(Vec2(0.0f, 0.0f)), mouse_pos_screen(Vec2(0.0f, 0.0f)), show_pipeline(false) {};
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

// Default tag capacity is pretty arbitrary at this point
constexpr size_t kDefaultTagCapacity = 1000;
TagGod::TagGod() :
    tags(DynamicArray<String>(kDefaultTagCapacity)),
    index(HashMap<String, TagId>(kDefaultTagCapacity)) {}

// Gets the id for a tag or creates a new tag id for it if it is not currently
// in its index
TagId TagGod::GetTagId(String tag) {
    TagId* id = &this->index[tag];
    if (id) {
        return *id;
    }

    TagId new_id = this->tags.Length();
    this->index.Set(tag, new_id);
    this->tags.Push(tag);

    return new_id;
}

String TagGod::GetTag(TagId id) {
    return this->tags[id];
}