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

    std::wstring wide_string = std::wstring(text.chars.Data(), text.chars.End());
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

Path PathBuilder::Build(DXState *dx) {
    HRESULT hr = S_OK;

    if (this->has_open_figure) {
        this->geometry_sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }

    hr = this->geometry_sink->Close();
    hr = this->geometry_sink->Release();

    ExitOnFailure(hr);

    return Path(this->geometry, dx);
}

Path::Path(ID2D1PathGeometry *geometry, DXState *dx) :
    geometry(geometry),
    transform(Transformation()),
    collection(0),
    tags(DynamicArray<String>(0)) {
    this->RealizeGeometry(dx);
};

void Path::Free() {
    this->tags.FreeAll();
    this->low_fidelity->Release();
    this->transformed_geometry->Release();
    this->geometry->Release();
}

Path Path::CreateLine(Vec2 from, Vec2 to, DXState *dx) {
    PathBuilder builder = PathBuilder(dx);
    builder.Move(from);
    builder.Line(to);

    return builder.Build(dx);
}

Path Path::CreateRect(Vec2 pos, Vec2 size, DXState *dx) {
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
    return builder.Build(dx);
}

Path Path::CreateCircle(Vec2 center, float radius, DXState *dx) {
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

    return builder.Build(dx);
}

D2D1_RECT_F Path::Bound() {
    return ShapeBound(this);
}

D2D1_RECT_F Path::OriginalBound() {
    return ShapeOriginalBound(this);
}

Vec2 Path::Center() {
    return ShapeCenter(this);
}

Vec2 Path::OriginalCenter() {
    return ShapeOriginalCenter(this);
}

D2D1::Matrix3x2F Path::TransformMatrix() {
    return ShapeTransformMatrix(this);
}

void Path::SetPos(Vec2 to) {
    D2D1_RECT_F bound = this->Bound();
    Vec2 current_pos  = Vec2(bound.left, bound.top);

    Vec2 diff = current_pos - to;
    this->transform.translation -= diff;
}

constexpr float kFloatLowFidelity = 1.0f;
void Path::RealizeGeometry(DXState* dx) {
    HRESULT hr;

    float dpix, dpiy;
    dx->d2_device_context->GetDpi(&dpix, &dpiy);

    hr = dx->factory->CreateTransformedGeometry(
        this->geometry,
        this->TransformMatrix(),
        &this->transformed_geometry
    );
    ExitOnFailure(hr);


    hr = dx->d2_device_context->CreateStrokedGeometryRealization(this->transformed_geometry, kFloatLowFidelity, kHairline, NULL, &this->low_fidelity);
    ExitOnFailure(hr);
}