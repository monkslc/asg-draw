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
    return this->documents.GetPtr(this->active_doc);
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
    geometries(HashMap<size_t, ID2D1PathGeometry*>(estimated_shapes)),

    transformed_geometries_index(HashMap<size_t, size_t>(estimated_shapes)),
    reverse_transformed_geometries_index(HashMap<size_t, size_t>(estimated_shapes)),
    transformed_geometries(DynamicArray<ID2D1TransformedGeometry*>(estimated_shapes)),

    reverse_collections_index(HashMap<size_t, DynamicArray<size_t>>(estimated_shapes)),
    collections(HashMap<size_t, size_t>(estimated_shapes)),

    tags_index(HashMap<size_t, DynamicArray<String>>(estimated_shapes)),
    reverse_tags_index(HashMap<String, DynamicArray<size_t>>(estimated_shapes)),

    low_fidelities(DynamicArray<ID2D1GeometryRealization*>(estimated_shapes)),
    low_fidelities_index(HashMap<size_t, size_t>(estimated_shapes)),

    transformations(HashMap<size_t, Transformation>(estimated_shapes)),
    texts(DynamicArray<Text>(10)),
    active_shapes(DynamicArray<ActiveShape>(5)),

    pipeline_shapes(DynamicArray<Shape>(estimated_shapes)) {};

void Document::Free() {
    this->texts.FreeAll();

    // Geometry and transformations
    for (auto i=0; i<this->geometries.Capacity(); i++) {
        auto slot = this->geometries.Slot(i);
        for (auto j=0; j<slot->Length(); j++) {
            auto entry = slot->GetPtr(j);
            entry->value->Release();
        }
    }
    this->geometries.Free();
    this->transformations.Free();

    // Transformed geometries
    for (auto i=0; i<this->transformed_geometries.Length(); i++) {
        this->transformed_geometries.Get(i)->Release();
    }
    this->transformed_geometries.Free();
    this->transformed_geometries_index.Free();
    this->reverse_transformed_geometries_index.Free();

    // Low fidelities
    for (auto i=0; i<this->low_fidelities.Length(); i++) {
        this->low_fidelities.Get(i)->Release();
    }
    this->low_fidelities.Free();
    this->low_fidelities_index.Free();

    // Collections
    this->collections.Free();
    this->reverse_collections_index.FreeValues();
    this->reverse_collections_index.Free();

    // Tags
    // Tags index Strings don't have to be freed because they will be freed in reverse_tags_index
    this->tags_index.FreeValues();
    this->tags_index.Free();
    this->reverse_tags_index.FreeKeyValues();
    this->reverse_tags_index.Free();

    this->active_shapes.Free();
}

void Document::AddNewPath(Path p) {
    size_t next_collection = this->NextCollection();
    size_t next_id = this->NextId();

    this->geometries.Set(next_id, p.geometry);

    size_t lf_array_index = this->low_fidelities.Length();
    this->low_fidelities.Push(p.low_fidelity);
    this->low_fidelities_index.Set(next_id, lf_array_index);

    size_t tg_array_index = this->transformed_geometries.Length();
    this->reverse_transformed_geometries_index.Set(tg_array_index, next_id);
    this->transformed_geometries_index.Set(next_id, tg_array_index);
    this->transformed_geometries.Push(p.transformed_geometry);

    this->transformations.Set(next_id, p.transform);

    this->collections.Set(next_id, next_collection);
    DynamicArray<size_t>* collection = this->reverse_collections_index.GetPtrOrDefault(next_collection);
    collection->Push(next_id);
}

size_t Document::NextCollection() {
    size_t next = this->next_collection;
    this->next_collection++;
    return next;
}

size_t Document::NextId() {
    size_t next = this->next_id;
    this->next_id++;
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

    for (auto i=0; i<this->transformed_geometries.Length(); i++) {
        ID2D1TransformedGeometry* geo = this->transformed_geometries.Get(i);
        D2D1_RECT_F bound;
        HRESULT hr = geo->GetBounds(NULL, &bound);
        ExitOnFailure(hr);
        if (EntirelyContains(&selection, &bound)) {
            size_t shape_id = *this->reverse_transformed_geometries_index.GetPtr(i);
            this->active_shapes.Push(ActiveShape(shape_id));
        }
    }
}

void Document::SelectShape(Vec2 screen_pos) {
    this->active_shapes.Clear();

    D2D1_POINT_2F point = screen_pos.D2Point();

    D2D1::Matrix3x2F doc_to_screen = this->view.DocumentToScreenMat();

    for (auto i=0; i<this->transformed_geometries.Length(); i++) {
        ID2D1TransformedGeometry* geo = this->transformed_geometries.Get(i);
        BOOL contains_point;
        HRESULT hr = geo->StrokeContainsPoint(point, kHairline, NULL, &doc_to_screen, &contains_point);
        ExitOnFailure(hr);
        if (contains_point) {
            size_t shape_id = *this->reverse_transformed_geometries_index.GetPtr(i);
            this->active_shapes.Push(ActiveShape(shape_id));
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

D2D1_RECT_F Document::GeometryBound(size_t id) {
    D2D1_RECT_F bound;

    size_t index = *this->transformed_geometries_index.GetPtr(id);
    ID2D1TransformedGeometry *geo = *this->transformed_geometries.GetPtr(index);
    geo->GetBounds(NULL, &bound);

    return bound;
}

constexpr float kFloatLowFidelity = 1.0f;
void Document::RealizeGeometry(DXState *dx, size_t id) {
    HRESULT hr;

    float dpix, dpiy;
    dx->d2_device_context->GetDpi(&dpix, &dpiy);

    ID2D1TransformedGeometry** transformed_geometry = this->transformed_geometries.GetPtr(id);

    ID2D1Geometry* source_geometry;
    (*transformed_geometry)->GetSourceGeometry(&source_geometry);

    Transformation* transform = this->transformations.GetPtr(id);
    hr = dx->factory->CreateTransformedGeometry(
        source_geometry,
        transform->Matrix(GeometryCenter(source_geometry)),
        transformed_geometry
    );

    source_geometry->Release();

    size_t low_fidelity_index = *this->low_fidelities_index.GetPtr(id);
    ID2D1GeometryRealization** realization = this->low_fidelities.GetPtr(low_fidelity_index);

    hr = dx->d2_device_context->CreateStrokedGeometryRealization(*transformed_geometry, kFloatLowFidelity, kHairline, NULL, realization);
}

void Document::CollectActiveShapes() {
    size_t collection = this->NextCollection();
    for (auto i=0; i<this->active_shapes.Length(); i++) {
        ActiveShape shape = this->active_shapes.Get(i);
        this->SetCollection(shape.id, collection);
    }
}

void Document::SetCollection(size_t shape_id, size_t new_collection_id) {
    size_t old_collection_id = *this->collections.GetPtr(shape_id);
    this->collections.Set(shape_id, new_collection_id);

    DynamicArray<size_t>* new_collection = this->reverse_collections_index.GetPtrOrDefault(new_collection_id);
    new_collection->Push(shape_id);

    DynamicArray<size_t>* old_collection = this->reverse_collections_index.GetPtr(old_collection_id);

    if (old_collection->Length() == 1) {
        old_collection->Free();
        this->reverse_collections_index.Remove(old_collection_id);
    } else {
        old_collection->Remove(shape_id);
    }
}

void Document::AddTag(size_t shape_id, char* tag_cstr) {
    // Currently AddTag creates one allocation for both tag indexes and places a spearate string in each
    // which means only one of them should get freed If this becomes a problem tags can be abstracted out
    // into their own array and referred to using indexes
    String tag = String(tag_cstr);

    DynamicArray<String>* tags = this->tags_index.GetPtrOrDefault(shape_id);
    tags->Push(tag);

    DynamicArray<size_t>* shape_ids = this->reverse_tags_index.GetPtrOrDefault(tag);
    shape_ids->Push(shape_id);
}

void Document::AutoCollect() {
    auto begin = std::chrono::high_resolution_clock::now();

    size_t memory_estimation = this->transformed_geometries.Length() * 2 * sizeof(RectNamed);
    LinearAllocatorPool allocator = LinearAllocatorPool(memory_estimation);

    auto shape_bounds = DynamicArrayEx<RectNamed, LinearAllocatorPool>(this->transformed_geometries.Length(), &allocator);
    for (auto i=0; i<this->transformed_geometries.Length(); i++) {
        ID2D1TransformedGeometry* geo = this->transformed_geometries.Get(i);
        size_t shape_id               = *this->reverse_transformed_geometries_index.GetPtr(i);

        Rect bounds = GetBounds(geo);
        shape_bounds.Push(RectNamed(bounds, shape_id), &allocator);
    }

    std::sort(shape_bounds.Data(), shape_bounds.End(), SortRectPositionXY<RectNamed>);

    auto new_collections = DynamicArrayEx<RectNamed, LinearAllocatorPool>(this->transformed_geometries.Length(), &allocator);
    auto search_from = 0;
    for (auto i=0; i<shape_bounds.Length(); i++) {
        RectNamed *shape = shape_bounds.GetPtr(i);

        bool found_fit = false;
        for (auto j=search_from; j<new_collections.Length(); j++) {
            RectNamed* collection = new_collections.GetPtr(j);
            if (collection->rect.Contains(&shape->rect)) {
                this->SetCollection(shape->id, collection->id);
                found_fit = true;
                break;
            }

            // Since the shapes are sorted, the collection will not be able to hold any of the
            // the shapes that follow if shape.left is greater than the collection right
            if (shape->Left() > collection->Right()) {
                search_from = j;
            }
        }

        if (!found_fit) {
            size_t next_collection = this->NextCollection();
            this->SetCollection(shape->id, next_collection);
            new_collections.Push(RectNamed(shape->rect, next_collection), &allocator);
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