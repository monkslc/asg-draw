#ifndef SVIGGY_H
#define SVIGGY_H

#include <dwrite.h>
#include <d2d1.h>
#include <string>
#include <vector>

#include <system_error>

#include "d2debug.hpp"

constexpr float kPixelsPerInch = 50;
constexpr float kScaleDelta = 0.1;
constexpr float kTranslationDelta = 0.125;
constexpr float kHairline = 0.03;

class Vec2 {
    public:
    float x,y;
    Vec2(float x, float y);
    Vec2(D2D1_POINT_2F p);

    D2D1_POINT_2F D2Point();
    Vec2 operator+(Vec2 &b);
    Vec2& operator+=(Vec2 &b);
    Vec2 operator-(Vec2 &b);
    Vec2& operator-=(Vec2 &b);
};

class Rect {
    public:
    Vec2 pos, size;
    Rect(float x, float y, float width, float height);
    Rect(Vec2 pos, Vec2 size);

    float X();
    float Y();
    float Width();
    float Height();
    float Left();
    float Top();
    float Right();
    float Bottom();
};

class Text {
    public:
    Vec2 pos;
    std::string text;
    Text(float x, float y, std::string text);
    Text(Vec2 pos, std::string text);

    float X();
    float Y();
};

class Line {
    public:
    Vec2 start, end;
    Line(Vec2 start, Vec2 end);
    Line(float x1, float y1, float x2, float y2);
};

class Poly {
    public:
    std::vector<Vec2> points;
    Poly();
    Poly(std::vector<Vec2> points);
};

class Circle {
    public:
    Vec2 center;
    float radius;
    Circle(Vec2 center, float radius);
    Circle(float x, float y, float radius);
};

constexpr float kPathCommandMove   = (float) 'M';
constexpr float kPathCommandCubic  = (float) 'C';
constexpr float kPathCommandLine   = (float) 'L';

class Path {
    public:
    std::vector<float> commands;
    Path(std::vector<float> commands);
};

class Document {
    public:
    std::vector<Rect> shapes;
    std::vector<Text> texts;
    std::vector<Line> lines;
    std::vector<Poly> polygons;
    std::vector<Circle> circles;
    std::vector<Path> paths;
    Document() {};
};

class View {
    public:
    Vec2 start;
    Vec2 mouse_pos_screen;
    float scale = 1.0;
    View();

    Vec2 GetDocumentPosition(Vec2 screen_pos);
    Vec2 MousePos();
    void Scale(bool in);
    D2D1::Matrix3x2F DocumentToScreenMat();
    D2D1::Matrix3x2F ScreenToDocumentMat();
};

class D2State {
    public:
    ID2D1Factory* factory;
    IDWriteFactory *write_factory;
    IDWriteTextFormat *text_format;
    IDWriteTextFormat *debug_text_format;
    ID2D1PathGeometry *geometry;
    ID2D1GeometrySink *geometry_sink;

    ID2D1HwndRenderTarget* renderTarget;
    ID2D1SolidColorBrush* lightSlateGrayBrush;
    ID2D1SolidColorBrush* cornflowerBlueBrush;
    ID2D1SolidColorBrush* blackBrush;
    ID2D1SolidColorBrush* debugBrush;

    D2Debug debug_info;

    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources(HWND hwnd);
    void DiscardDeviceResources();
    void Resize(UINT width, UINT height);
    HRESULT Render(Document *doc, View *view);
    void RenderRects(Document *doc, View *view);
    void RenderLines(Document *doc, View *view);
    void RenderPolygons(Document *doc, View *view);
    void RenderCircles(Document *doc, View *view);
    void RenderPaths(Document *doc, View *view);
    void RenderText(Document *doc, View *view);
    void RenderGridLines();
    void RenderDebugInfo();
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateDebugConsole();

#endif