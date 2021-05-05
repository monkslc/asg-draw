#ifndef SVIGGY_H
#define SVIGGY_H

#include <dwrite.h>
#include <d2d1.h>
#include <vector>

#include <system_error>

constexpr float kPixelsPerInch = 50;
constexpr float kScaleDelta = 0.1;
constexpr float kTranslationDelta = 0.125;
constexpr float kHairline = 0.05;

class Vec2 {
    public:
    float x,y;
    Vec2(float x, float y) : x(x), y(y) {};
    Vec2(D2D1_POINT_2F p) : x(p.x), y(p.y) {};
    D2D1_POINT_2F D2Point() {
        return D2D1::Point2F(x, y);
    };

    Vec2 operator+(Vec2 &b) {
        return Vec2(this->x + b.x, this->y + b.y);
    }

    Vec2& operator+=(Vec2 &b) {
        this->x += b.x;
        this->y += b.y;

        return *this;
    }

    Vec2 operator-(Vec2 &b) {
        return Vec2(this->x - b.x, this->y - b.y);
    }

    Vec2& operator-=(Vec2 &b) {
        this->x -= b.x;
        this->y -= b.y;

        return *this;
    }
};

class Rect {
    public:
    Vec2 pos, size;
    Rect(float x, float y, float width, float height) : pos(Vec2(x, y)), size(Vec2(width, height)) {};
    Rect(Vec2 pos, Vec2 size) : pos(pos), size(size) {};

    float X() {
        return this->pos.x;
    }

    float Y() {
        return this->pos.y;
    }

    float Width() {
        return this->size.x;
    }

    float Height() {
        return this->size.y;
    }

    float Left() {
        return this->pos.x;
    }

    float Top() {
        return this->pos.y;
    }

    float Right() {
        return this->pos.x + this->size.x;
    }

    float Bottom() {
        return this->pos.y + this->size.y;
    }
};

class Text {
    public:
    Vec2 pos;
    std::string text;
    Text(float x, float y, std::string text) : pos(Vec2(x, y)), text(text) {};
    Text(Vec2 pos, std::string text) : pos(pos), text(text) {};

    float X() {
        return this->pos.x;
    }

    float Y() {
        return this->pos.y;
    }
};

class Line {
    public:
    Vec2 start, end;
    Line(Vec2 start, Vec2 end) : start(start), end(end) {};
    Line(float x1, float y1, float x2, float y2) : start(Vec2(x1, y1)), end(Vec2(x2, y2)) {};
};

class Poly {
    public:
    std::vector<Vec2> points;
    Poly() {};
    Poly(std::vector<Vec2> points) : points(points) {};
};

class Circle {
    public:
    Vec2 center;
    float radius;
    Circle(Vec2 center, float radius) : center(center), radius(radius) {};
    Circle(float x, float y, float radius) : center(Vec2(x, y)), radius(radius) {};
};

class Document {
    public:
    std::vector<Rect> shapes;
    std::vector<Text> texts;
    std::vector<Line> lines;
    std::vector<Poly> polygons;
    std::vector<Circle> circles;
    Document() {};
};

class View {
    public:
    Vec2 start;
    Vec2 mouse_pos_screen;
    float scale = 1.0;
    View() : start(Vec2(0.0f, 0.0f)), mouse_pos_screen(Vec2(0.0f, 0.0f)) {};
    Vec2 GetDocumentPosition(Vec2 screen_pos) {
        return this->ScreenToDocumentMat().TransformPoint(screen_pos.D2Point());
    }

    Vec2 MousePos() {
        return this->GetDocumentPosition(this->mouse_pos_screen);
    }

    void Scale(bool in) {
        // We preserve the mouse document position when scaling in and out
        Vec2 original_mouse_pos = this->MousePos();

        float delta = in ? 1 + kScaleDelta : 1-kScaleDelta;
        this->scale *= delta;

        Vec2 mouse_after_scale = this->MousePos();
        Vec2 mouse_change = original_mouse_pos - mouse_after_scale;

        this->start += mouse_change;
    }

    D2D1::Matrix3x2F DocumentToScreenMat() {
        D2D1::Matrix3x2F scale_matrix = D2D1::Matrix3x2F::Scale(
            this->scale * kPixelsPerInch,
            this->scale * kPixelsPerInch,
            D2D1_POINT_2F()
        );

        D2D1::Matrix3x2F translation_matrix = D2D1::Matrix3x2F::Translation(-this->start.x, -this->start.y);

        return translation_matrix * scale_matrix;
    }

    D2D1::Matrix3x2F ScreenToDocumentMat() {
        D2D1::Matrix3x2F mat = this->DocumentToScreenMat();
        mat.Invert();
        return mat;
    }
};

class D2State {
    public:
    ID2D1Factory* factory;
    IDWriteFactory *write_factory;
    IDWriteTextFormat *text_format;
    ID2D1PathGeometry *geometry;
    ID2D1GeometrySink *geometry_sink;

    ID2D1HwndRenderTarget* renderTarget;
    ID2D1SolidColorBrush* lightSlateGrayBrush;
    ID2D1SolidColorBrush* cornflowerBlueBrush;
    ID2D1SolidColorBrush* blackBrush;
    ID2D1SolidColorBrush* debugBrush;
    HRESULT CreateDeviceIndependentResources() {
        HRESULT hr;
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &this->factory);

        if (FAILED(hr)) {
            return hr;
        }

        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(write_factory),
            reinterpret_cast<IUnknown **>(&write_factory)
        );

        if (FAILED(hr)) {
            return hr;
        }

         hr = write_factory->CreateTextFormat(
            L"Arial",
            NULL,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.0 / kPixelsPerInch,
            L"",
            &text_format
        );

        if (FAILED(hr)) {
            return hr;
        }

        text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

        hr = this->factory->CreatePathGeometry(&this->geometry);
        if (FAILED(hr)) {
            return hr;
        }

        hr = this->geometry->Open(&this->geometry_sink);
        if (FAILED(hr)) {
            return hr;
        }

        return hr;
    }

    HRESULT CreateDeviceResources(HWND hwnd) {
        HRESULT hr = S_OK;
        if (renderTarget) {
            return hr;
        }

        RECT rc;
        GetClientRect(hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top
        );

        hr = factory->CreateHwndRenderTarget(
            // TODO: what target properties do we want?
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            &renderTarget
        );

        if (FAILED(hr)) {
            return hr;
        }

        hr = renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::LightSlateGray),
            &lightSlateGrayBrush
        );

        if (FAILED(hr)) {
            return hr;
        }

        hr = renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
            &cornflowerBlueBrush
        );

        if (FAILED(hr)) {
            return hr;
        }

        renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::Black),
            &blackBrush
        );

        if (FAILED(hr)) {
            return hr;
        }

        renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::Red),
            &debugBrush
        );

        return hr;
    }

    void DiscardDeviceResources() {
        if (renderTarget != NULL) {
            renderTarget->Release();
            renderTarget = NULL;
        }

        if (lightSlateGrayBrush != NULL) {
            lightSlateGrayBrush->Release();
            lightSlateGrayBrush = NULL;
        }

        if (cornflowerBlueBrush != NULL) {
            cornflowerBlueBrush->Release();
            cornflowerBlueBrush = NULL;
        }
    }

    void Resize(UINT width, UINT height) {
        if (this->renderTarget) {
            this->renderTarget->Resize(D2D1::SizeU(width, height));
        }
    }

    HRESULT Render(Document *doc, View *view) {
        this->renderTarget->BeginDraw();
        this->renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

        this->RenderGridLines();

        this->renderTarget->SetTransform(view->DocumentToScreenMat());

        D2D1_SIZE_F rtSize = this->renderTarget->GetSize();
        int width = static_cast<int>(rtSize.width);
        int height = static_cast<int>(rtSize.height);

        this->RenderRects(doc, view);
        this->RenderLines(doc, view);
        this->RenderPolygons(doc, view);
        this->RenderCircles(doc, view);
        this->RenderText(doc, view);

        HRESULT hr = this->renderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            this->DiscardDeviceResources();
        } else if (FAILED(hr)) {
            printf("Drawing Failed%s\n", std::system_category().message(hr).c_str());
        }

        return hr;
    }

    void RenderRects(Document *doc, View *view) {
        for (auto &shape : doc->shapes) {
            D2D1_RECT_F rectangle = D2D1::RectF(
                shape.Left(),
                shape.Top(),
                shape.Right(),
                shape.Bottom()
            );
            this->renderTarget->DrawRectangle(&rectangle, this->blackBrush, kHairline);
        }
    }

    void RenderLines(Document *doc, View *view) {
        for (auto &line : doc->lines) {
            this->renderTarget->DrawLine(line.start.D2Point(), line.end.D2Point(), this->blackBrush, kHairline, NULL);
        }
    }

    void RenderPolygons(Document *doc, View *view) {
        this->geometry_sink->SetFillMode(D2D1_FILL_MODE_WINDING);
        for (auto &poly : doc->polygons) {
            if (poly.points.empty()) continue;

            Vec2 start = poly.points[0];
            this->geometry_sink->BeginFigure(start.D2Point(), D2D1_FIGURE_BEGIN_FILLED);

            for (auto i=1; i<poly.points.size(); i++) {
                Vec2 next = poly.points[i];
                this->geometry_sink->AddLine(next.D2Point());
            }

            this->geometry_sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        }

        this->geometry_sink->Close();
        this->renderTarget->DrawGeometry(this->geometry, this->blackBrush, kHairline);
    }

    void RenderCircles(Document *doc, View *view) {
        for (auto &circle : doc->circles) {
            D2D1_ELLIPSE ellipse = D2D1::Ellipse(circle.center.D2Point(), circle.radius, circle.radius);
            this->renderTarget->DrawEllipse(ellipse, this->blackBrush, kHairline, nullptr);
        }
    }

    void RenderText(Document *doc, View *view) {
        for (auto &text : doc->texts) {
            std::wstring wide_string = std::wstring(text.text.begin(), text.text.end());
            this->renderTarget->DrawText(
                wide_string.c_str(),
                wide_string.size(),
                this->text_format,
                // TODO: not sure what to set for the right and bottom parameters
                // Right now its 100 but this will break if we have something wider
                D2D1::RectF(text.X(), text.Y(), 100, 100),
                this->blackBrush
            );
        }
    }

    void RenderGridLines() {
        this->renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        D2D1_SIZE_F size = renderTarget->GetSize();

        int width  = static_cast<int>(size.width);
        int height = static_cast<int>(size.height);

        for (int x = 0; x < width; x += 10) {
                renderTarget->DrawLine(
                    D2D1::Point2F(static_cast<FLOAT>(x), 0.0f),
                    D2D1::Point2F(static_cast<FLOAT>(x), size.height),
                    cornflowerBlueBrush,
                    0.1f
                );
            }

        for (int y = 0; y < height; y += 10) {
            renderTarget->DrawLine(
                D2D1::Point2F(0.0f, static_cast<FLOAT>(y)),
                D2D1::Point2F(size.width, static_cast<FLOAT>(y)),
                cornflowerBlueBrush,
                0.1f
            );
        }

    }
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateDebugConsole();

#endif