#include <d2d1.h>
#include <d2d1_2.h>

#include "ds.hpp"
#include "shapes.hpp"
#include "sviggy.hpp"

// TODO: errors need to be handled in this file when creating geometry
// Maybe a messagebox pops up to the user asking them if they want to close
// the application

Transformation::Transformation() : translation(Vec2(0.0f, 0.0f)), scale(Vec2(1.0, 1.0)), rotation(0.0f) {};
D2D1::Matrix3x2F Transformation::Matrix(Vec2 center) {
    D2D1::Matrix3x2F rotation = D2D1::Matrix3x2F::Rotation(this->rotation, center.D2Point());
    D2D1::Matrix3x2F translation = D2D1::Matrix3x2F::Translation(this->translation.Size());
    D2D1::Matrix3x2F scale = D2D1::Matrix3x2F::Scale(this->scale.Size(), center.D2Point());
    return scale * rotation * translation;
};

Text::Text(Vec2 pos, String text, DXState* dx) : pos(pos), text(text), transform(Transformation()) {
    HRESULT hr;

    hr = dx->write_factory->CreateTextFormat(
        L"Arial",
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0 / kPixelsPerInch,
        L"",
        &this->format
    );

    this->format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    this->format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    std::wstring wide_string = std::wstring(text.Data(), text.End());
    dx->write_factory->CreateTextLayout(
        wide_string.c_str(),
        wide_string.size(),
        this->format,
        10, // TODO: Not sure what these (width, height) should be set to. It's not clear
        10, // to me how I determine the width and height of the text at this point
        &this->layout
    );
};

void Text::Free() {
    this->layout->Release();
    this->format->Release();
}

float Text::X() {
    return this->pos.x;
}

float Text::Y() {
    return this->pos.y;
}

PathBuilder::PathBuilder(DXState *dx) : has_open_figure(false) {
    HRESULT hr;

    hr = dx->factory->CreatePathGeometry(&this->geometry);
    ExitOnFailure(hr);

    hr = this->geometry->Open(&this->geometry_sink);
    ExitOnFailure(hr);

    geometry_sink->SetFillMode(D2D1_FILL_MODE_WINDING);
}

ShapeData PathBuilder::CreateLine(Vec2 from, Vec2 to, DXState *dx) {
    PathBuilder builder = PathBuilder(dx);
    builder.Move(from);
    builder.Line(to);

    return builder.BuildPath(dx);
}

ShapeData PathBuilder::CreateRect(Vec2 pos, Vec2 size, DXState *dx) {
    float left  = pos.x;
    float right = pos.x + size.x;
    float top   = pos.y;
    float bot   = pos.y + size.y;

    PathBuilder builder = PathBuilder(dx);

    builder.Move(Vec2(left,  top));
    builder.Line(Vec2(right, top));
    builder.Line(Vec2(right, bot));
    builder.Line(Vec2(left,  bot));

    builder.Close();
    return builder.BuildPath(dx);
}

ShapeData PathBuilder::CreateCircle(Vec2 center, float radius, DXState *dx) {
    float startx = center.x - radius;
    float endx   = center.x + radius;

    Vec2 start = Vec2(startx, center.y);
    Vec2 end   = Vec2(endx,   center.y);

    Vec2 size = Vec2(radius, radius);

    // TODO: Right now we create a circle with two arcs
    // This doesn't seem like a great solution but I'm not sure how else
    // to do it so I'm leaving this TODO as a reminder to come back later
    PathBuilder builder = PathBuilder(dx);
    builder.Move(start);
    builder.Arc(end, size, 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE);
    builder.Move(start);
    builder.Arc(end, size, 0.0f, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE);

    return builder.BuildPath(dx);
}

void PathBuilder::Move(Vec2 to) {
    if (this->has_open_figure) {
        geometry_sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }

    geometry_sink->BeginFigure(to.D2Point(), D2D1_FIGURE_BEGIN_FILLED);
    this->has_open_figure = true;
}

void PathBuilder::Line(Vec2 to) {
   this->geometry_sink->AddLine(to.D2Point());
}

void PathBuilder::Cubic(Vec2 c1, Vec2 c2, Vec2 end) {
    D2D1_BEZIER_SEGMENT bezier = D2D1::BezierSegment(c1.D2Point(), c2.D2Point(), end.D2Point());
    geometry_sink->AddBezier(bezier);
}

void PathBuilder::Arc(Vec2 end, Vec2 size, float rot, D2D1_SWEEP_DIRECTION direction) {
    D2D1_ARC_SEGMENT arc = D2D1_ARC_SEGMENT {
        end.D2Point(),
        size.Size(),
        rot,
        direction,
        D2D1_ARC_SIZE_LARGE,
    };
    this->geometry_sink->AddArc(arc);
}

void PathBuilder::Close() {
    if (this->has_open_figure) {
        this->geometry_sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        this->has_open_figure = false;
    }
}

ShapeData PathBuilder::BuildPath(DXState *dx) {
    HRESULT hr;

    if (this->has_open_figure) {
        this->geometry_sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }

    hr = this->geometry_sink->Close();
    ExitOnFailure(hr);

    hr = this->geometry_sink->Release();
    ExitOnFailure(hr);

    return ShapeData(this->geometry);
}

ShapeData::ShapeData(ID2D1Geometry* geometry) :
    transform(Transformation()),
    geometry(geometry)
    {};

D2D1_MATRIX_3X2_F ShapeData::TransformMatrix() {
    Vec2 center = GeometryCenter(this->geometry);
    return this->transform.Matrix(center);
};

Paths::Paths(size_t estimated_cap) :
    shapes(DynamicArray<ShapeData>(estimated_cap)),
    transformed_geometries(DynamicArray<ID2D1TransformedGeometry*>(estimated_cap)),
    low_fidelities(DynamicArray<ID2D1GeometryRealization*>(estimated_cap)),
    index(HashMap<PathId, size_t>(estimated_cap)),
    reverse_index(DynamicArray<PathId>(estimated_cap)),
    next_id(0) {};

void Paths::Free() {
    this->shapes.Free();

    this->transformed_geometries.ReleaseAll();
    this->transformed_geometries.Free();

    this->low_fidelities.ReleaseAll();
    this->low_fidelities.Free();

    this->index.Free();

    this->reverse_index.Free();
};

PathId Paths::AddPath(ShapeData path) {
    PathId id    = this->NextId();
    size_t index = this->Length();

    this->shapes.Push(path);

    this->transformed_geometries.Push(NULL);
    this->low_fidelities.Push(NULL);

    this->index.Set(id, index);
    this->reverse_index.Push(id);

    return id;
};

PathId Paths::NextId() {
    size_t next = this->next_id;
    this->next_id++;
    return next;
}

size_t Paths::Length() {
    // SInce all fo the arrays should be the same size arbitrarily pick one of them for the length
    return this->shapes.Length();
}


void Paths::RealizeGeometry(DXState *dx, PathId id) {
    HRESULT hr;
    size_t index = *this->index.GetPtr(id);

    ShapeData* path                                 = this->shapes.GetPtr(index);
    ID2D1TransformedGeometry** transformed_geometry = this->transformed_geometries.GetPtr(index);
    ID2D1GeometryRealization** low_fidelity         = this->low_fidelities.GetPtr(index);

    CreateGeometryRealizations(path, transformed_geometry, low_fidelity, dx);
}

void Paths::RealizeAllGeometry(DXState *dx) {
    for (auto i=0; i<this->Length(); i++) {
        ShapeData* path                                 = this->shapes.GetPtr(i);
        ID2D1TransformedGeometry** transformed_geometry = this->transformed_geometries.GetPtr(i);
        ID2D1GeometryRealization** low_fidelity         = this->low_fidelities.GetPtr(i);

        CreateGeometryRealizations(path, transformed_geometry, low_fidelity, dx);
    }
}

ShapeData* Paths::GetShapeData(PathId id) {
    size_t index = *this->index.GetPtr(id);
    return this->shapes.GetPtr(index);
}

ID2D1TransformedGeometry** Paths::GetTransformedGeometry(PathId id) {
    size_t index = *this->index.GetPtr(id);
    return this->transformed_geometries.GetPtr(id);
}

ID2D1GeometryRealization** Paths::GetLowFidelity(PathId id) {
    size_t index = *this->index.GetPtr(id);
    return this->low_fidelities.GetPtr(id);
}

Rect Paths::GetBounds(PathId id) {
    size_t index = *this->index.GetPtr(id);

    ID2D1TransformedGeometry* transformed_geo = this->transformed_geometries.Get(index);

    D2D1_RECT_F bounds;
    HRESULT hr = transformed_geo->GetBounds(NULL, &bounds);
    ExitOnFailure(hr);

    return Rect(&bounds);
}

Collections::Collections(size_t estimated_shapes) :
    collections(HashMap<PathId, CollectionId>(estimated_shapes)),
    reverse_collections_index(HashMap<CollectionId, DynamicArray<PathId>>(estimated_shapes)),
    next_id(0) {};

void Collections::Free() {
    this->collections.Free();

    this->reverse_collections_index.FreeValues();
    this->reverse_collections_index.Free();
}

CollectionId Collections::NextId() {
    size_t next = this->next_id;
    this->next_id++;
    return next;
}

CollectionId Collections::CreateCollectionForShape(PathId shape_id) {
    CollectionId collection_id = this->NextId();

    this->collections.Set(shape_id, collection_id);

    DynamicArray<PathId>* collection = this->reverse_collections_index.GetPtrOrDefault(collection_id);
    collection->Push(shape_id);

    return collection_id;
}

CollectionId Collections::GetCollectionId(PathId shape_id) {
    return *this->collections.GetPtr(shape_id);
}

void Collections::SetCollection(PathId shape_id, CollectionId collection_id) {
    size_t old_collection_id = this->GetCollectionId(shape_id);

    this->collections.Set(shape_id, collection_id);

    DynamicArray<size_t>* new_collection = this->reverse_collections_index.GetPtrOrDefault(collection_id);
    new_collection->Push(shape_id);

    DynamicArray<size_t>* old_collection = this->reverse_collections_index.GetPtr(old_collection_id);
    if (old_collection->Length() == 1) {
        old_collection->Free();
        this->reverse_collections_index.Remove(old_collection_id);
    } else {
        old_collection->Remove(shape_id);
    }
}

Tags::Tags(size_t estimated_shapes) :
    tags(HashMap<PathId, DynamicArray<String>>(estimated_shapes)),
    reverse_tags(HashMap<String, DynamicArray<PathId>>(estimated_shapes)) {};

void Tags::Free() {
    this->tags.FreeValues();
    this->tags.Free();

    this->reverse_tags.FreeValues();
    this->reverse_tags.Free();
}

DynamicArray<String>* Tags::GetTags(PathId shape_id) {
    return this->tags.GetPtr(shape_id);
}

void Tags::AddTag(size_t shape_id, char* tag_cstr) {
    String tag = String(tag_cstr);

    DynamicArray<String>* tags = this->tags.GetPtrOrDefault(shape_id);
    tags->Push(tag);

    DynamicArray<size_t>* shape_ids = this->reverse_tags.GetPtrOrDefault(tag);
    shape_ids->Push(shape_id);
}

Shape::Shape(ID2D1TransformedGeometry* geometry, Transformation transform) : geometry(geometry), transform(transform) {};

Rect::Rect(Vec2 pos, Vec2 size) : pos(pos), size(size) {};

Rect::Rect(D2D1_RECT_F* rect) :
 pos(Vec2(rect->left, rect->top)),
 size(Vec2(rect->right - rect->left, rect->bottom - rect->top)) {};

float Rect::Left() {
    return this->pos.x;
}

float Rect::Top() {
    return this->pos.y;
}

float Rect::Right() {
    return this->pos.x + this->size.x;
}

float Rect::Bottom() {
    return this->pos.y + this->size.y;
}

float Rect::Width() {
    return this->size.x;
}

float Rect::Height() {
    return this->size.y;
}

bool Rect::Contains(Rect *other) {
    return this->Left()   <= other->Left()  &&
           this->Top()    <= other->Top()   &&
           this->Right()  >= other->Right() &&
           this->Bottom() >= other->Bottom();
}

Rect Rect::Union(Rect* other) {
    float left   = std::min<float>(this->Left(),   other->Left());
    float top    = std::min<float>(this->Top(),    other->Top());
    float right  = std::max<float>(this->Right(),  other->Right());
    float bottom = std::max<float>(this->Bottom(), other->Bottom());

    Vec2 pos  = Vec2(left, top);
    Vec2 size = Vec2(right - left, top - bottom);

    return Rect(pos, size);
}

D2D1_RECT_F Rect::D2Rect() {
    return D2D1::RectF(this->Left(), this->Top(), this->Right(), this->Bottom());
}

RectNamed::RectNamed(Rect rect, size_t id) : rect(rect), id(id) {};

float RectNamed::Left() {
    return this->rect.Left();
}

float RectNamed::Top() {
    return this->rect.Top();
}

float RectNamed::Right() {
    return this->rect.Right();
}

float RectNamed::Bottom() {
    return this->rect.Bottom();
}

Vec2 GeometryCenter(ID2D1Geometry* geometry) {
   D2D1_RECT_F bound;
   HRESULT hr = geometry->GetBounds(NULL, &bound);
   ExitOnFailure(hr);

   float x = (bound.left + bound.right ) / 2.0f;
   float y = (bound.top  + bound.bottom) / 2.0f;

   return Vec2(x, y);
}

Transformation GetTranslationTo(Vec2 to, Rect* from) {
   Transformation transform = Transformation();

   Vec2 current_pos = Vec2(from->Left(), from->Top());
   Vec2 diff        = to - current_pos;

    transform.translation = diff;
    return transform;
}

Rect GetBounds(ID2D1TransformedGeometry* geo) {
    D2D1_RECT_F d2bound;
    HRESULT hr = geo->GetBounds(NULL, &d2bound);
    ExitOnFailure(hr);

    return Rect(&d2bound);
}

constexpr float kFloatLowFidelity = 1.0f;
void CreateGeometryRealizations(ShapeData* shape, ID2D1TransformedGeometry** transformed_geometry, ID2D1GeometryRealization** low_fidelity, DXState *dx) {
    HRESULT hr;

    if((*transformed_geometry)) {
        (*transformed_geometry)->Release();
    }

    if ((*low_fidelity)) {
        (*low_fidelity)->Release();
    }

    hr = dx->factory->CreateTransformedGeometry(shape->geometry, shape->TransformMatrix(), transformed_geometry);
    ExitOnFailure(hr);

    hr = dx->d2_device_context->CreateStrokedGeometryRealization(*transformed_geometry, kFloatLowFidelity, kHairline, NULL, low_fidelity);
    ExitOnFailure(hr);
}