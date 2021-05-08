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

// forward declarations
class D2State;

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
    ID2D1RectangleGeometry *geometry;
    Rect(Vec2 pos, Vec2 size, D2State *d2);

    D2D1_RECT_F Bound();
};

class Text {
    public:
    Vec2 pos;
    std::string text;
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    Text(Vec2 pos, std::string text, D2State *d2);

    float X();
    float Y();
};

class Circle {
    public:
    ID2D1EllipseGeometry *geometry;
    Circle(Vec2 center, float radius, D2State *d2);
};

constexpr float kPathCommandMove   = (float) 'M';
constexpr float kPathCommandCubic  = (float) 'C';
constexpr float kPathCommandLine   = (float) 'L';

class Path {
    public:
    std::vector<float> commands;
    ID2D1PathGeometry *geometry;
    Path(std::vector<float> commands, D2State *d2);
};

class Document {
    public:
    std::vector<Rect> shapes;
    std::vector<Text> texts;
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
    IDWriteTextFormat *debug_text_format;

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
    void RenderCircles(Document *doc, View *view);
    void RenderPaths(Document *doc, View *view);
    void RenderText(Document *doc, View *view);
    void RenderGridLines();
    void RenderDebugInfo();
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateDebugConsole();

#endif