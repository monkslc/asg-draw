#include <d2d1.h>

#include "shapes.hpp"
#include "sviggy.hpp"

// TODO: errors need to be handled in this file when creating geometry
// Maybe a messagebox pops up to the user asking them if they want to close
// the application

Transformation::Transformation() : translation(Vec2(0.0f, 0.0f)), rotation(0.0f) {};
D2D1::Matrix3x2F Transformation::Matrix(Vec2 center) {
    D2D1::Matrix3x2F rotation = D2D1::Matrix3x2F::Rotation(this->rotation, center.D2Point());
    D2D1::Matrix3x2F translation = D2D1::Matrix3x2F::Translation(this->translation.Size());
    return rotation * translation;
};

Text::Text(Vec2 pos, std::string text, DXState* dx) : pos(pos), text(text), transform(Transformation()) {
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

    std::wstring wide_string = std::wstring(text.begin(), text.end());
    dx->write_factory->CreateTextLayout(
        wide_string.c_str(),
        wide_string.size(),
        this->format,
        10, // TODO: Not sure what these (width, height) should be set to. It's not clear
        10, // to me how I determine the width and height of the text at this point
        &this->layout
    );

};

float Text::X() {
    return this->pos.x;
}

float Text::Y() {
    return this->pos.y;
}

Path::Path(std::vector<float> commands, DXState *dx) : commands(commands), transform(Transformation()) {
    HRESULT hr;
    hr = dx->factory->CreatePathGeometry(&this->geometry);

    ID2D1GeometrySink *geometry_sink;
    hr = this->geometry->Open(&geometry_sink);

    geometry_sink->SetFillMode(D2D1_FILL_MODE_WINDING);

    Vec2 start = Vec2(0.0f, 0.0f);
    auto i=0;

    // If the first command is a move, use that as the start. Otherwise (0, 0) will be the start
    // and will end up counting in geometry->Bounds()
    if (commands.size() > 2 && commands[0] == kPathCommandMove) {
        start = Vec2(commands[i+1], commands[i+2]);
        i += 3;
    }

    geometry_sink->BeginFigure(start.D2Point(), D2D1_FIGURE_BEGIN_FILLED);
    while (i < commands.size()) {
        if (commands[i] == kPathCommandMove) {
            Vec2 pos = Vec2(commands[i+1], commands[i+2]);
            i+=3;

            geometry_sink->EndFigure(D2D1_FIGURE_END_OPEN);
            geometry_sink->BeginFigure(pos.D2Point(), D2D1_FIGURE_BEGIN_FILLED);
            continue;
        }

        if (commands[i] == kPathCommandCubic) {
            Vec2 c1  = Vec2(commands[i+1], commands[i+2]);
            Vec2 c2  = Vec2(commands[i+3], commands[i+4]);
            Vec2 end = Vec2(commands[i+5], commands[i+6]);
            i += 7;

            D2D1_BEZIER_SEGMENT bezier = D2D1::BezierSegment(c1.D2Point(), c2.D2Point(), end.D2Point());
            geometry_sink->AddBezier(bezier);
            continue;
        }

        if (commands[i] == kPathCommandLine) {
            Vec2 to = Vec2(commands[i+1], commands[i+2]);
            i += 3;

            geometry_sink->AddLine(to.D2Point());
            continue;
        }

        if (commands[i] == kPathCommandArc) {
            Vec2 end  = Vec2(commands[i+1], commands[i+2]);
            Vec2 size = Vec2(commands[i+3], commands[i+4]);
            float rot = commands[i+5];

            D2D1_SWEEP_DIRECTION direction = commands[i+6] == kClockwise ?
                D2D1_SWEEP_DIRECTION_CLOCKWISE :
                D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;

            i += 7;

            geometry_sink->AddArc(D2D1_ARC_SEGMENT {
                end.D2Point(),
                size.Size(),
                rot,
                direction,
                D2D1_ARC_SIZE_LARGE,
            });
            continue;
        }

        printf("Unrecognized path command at %d of %.9f :(\n", i, commands[i]);
        i++;
        continue;
    }

    geometry_sink->EndFigure(D2D1_FIGURE_END_OPEN);
    geometry_sink->Close();
    geometry_sink->Release();
};

Path Path::CreateRect(Vec2 pos, Vec2 size, DXState *dx) {
    float left  = pos.x;
    float right = pos.x + size.x;
    float top   = pos.y;
    float bot   = pos.y + size.y;

    auto commands = std::vector<float>{
        kPathCommandMove, left,  top,
        kPathCommandLine, right, top,
        kPathCommandLine, right, bot,
        kPathCommandLine, left,  bot,
        kPathCommandLine, left,  top,
    };

    return Path(commands, dx);
}

Path Path::CreateCircle(Vec2 center, float radius, DXState *dx) {
    float startx = center.x - radius;
    float endx   = center.x + radius;

    // TODO: Right now we create a circle with two arcs
    // This doesn't seem like a great solution but I'm not sure how else
    // to do it so I'm leaving this TODO as a reminder to come back later
    auto commands = std::vector<float> {
        kPathCommandMove, startx, center.y,
        kPathCommandArc, endx, center.y, radius, radius, 0.0f, kClockwise,
        kPathCommandMove, startx, center.y,
        kPathCommandArc, endx, center.y, radius, radius, 0.0f, kCounterClockwise,
    };

    return Path(commands, dx);

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