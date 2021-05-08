#include <d2d1.h>

#include "sviggy.hpp"

// TODO: errors need to be handled in this file when creating geometry
// Maybe a messagebox pops up to the user asking them if they want to close
// the application

Rect::Rect(Vec2 pos, Vec2 size, D2State *d2) {
    D2D1_RECT_F rectangle = D2D1::RectF(
        pos.x,
        pos.y,
        pos.x + size.x,
        pos.y + size.y
    );
    d2->factory->CreateRectangleGeometry(rectangle, &this->geometry);
};

D2D1_RECT_F Rect::Bound() {
    D2D1_RECT_F bound;
    this->geometry->GetBounds(D2D1::Matrix3x2F::Identity(), &bound);
    return bound;
}

Text::Text(Vec2 pos, std::string text, D2State *d2) : pos(pos), text(text) {
    HRESULT hr;

    hr = d2->write_factory->CreateTextFormat(
        L"Arial",
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0 / kPixelsPerInch,
        L"",
        &this->format
    );

    std::wstring wide_string = std::wstring(text.begin(), text.end());
    d2->write_factory->CreateTextLayout(
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

Circle::Circle(Vec2 center, float radius, D2State *d2) {
    D2D1_ELLIPSE ellipse = D2D1::Ellipse(center.D2Point(), radius, radius);
    d2->factory->CreateEllipseGeometry(ellipse, &this->geometry);
};

Path::Path(std::vector<float> commands, D2State *d2) : commands(commands) {
    HRESULT hr;
    hr = d2->factory->CreatePathGeometry(&this->geometry);

    ID2D1GeometrySink *geometry_sink;
    hr = this->geometry->Open(&geometry_sink);

    geometry_sink->SetFillMode(D2D1_FILL_MODE_WINDING);
    geometry_sink->BeginFigure(Vec2(0.0f, 0.0f).D2Point(), D2D1_FIGURE_BEGIN_HOLLOW);
    auto i=0;
    while (i < commands.size()) {
        if (commands[i] == kPathCommandMove) {
            Vec2 pos = Vec2(commands[i+1], commands[i+2]);
            i+=3;

            geometry_sink->EndFigure(D2D1_FIGURE_END_OPEN);
            geometry_sink->BeginFigure(pos.D2Point(), D2D1_FIGURE_BEGIN_HOLLOW);
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

        printf("Unrecognized path command at %d of %.9f :(\n", i, commands[i]);
        i++;
        continue;
    }

    geometry_sink->EndFigure(D2D1_FIGURE_END_OPEN);
    geometry_sink->Close();
    // TODO: release geometry sink
};