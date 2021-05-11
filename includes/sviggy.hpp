#ifndef SVIGGY_H
#define SVIGGY_H

#include <d3d11.h>
#include <dxgi.h>
#include <dwrite.h>
#include <d2d1.h>
#include <string>
#include <vector>

#include <system_error>

#define RETURN_FAIL(hr) if(FAILED(hr)) return hr
// Subtract 1 from array size to avoid the null terminating character for b
#define STRNCMP(a, b) strncmp(a, b, ARRAYSIZE(b) - 1) == 0

constexpr float kPixelsPerInch = 50;
constexpr float kScaleDelta = 0.1;
constexpr float kTranslationDelta = 0.125;
constexpr float kHairline = 0.03;

// forward declarations
class DXState;

class Vec2 {
    public:
    float x,y;
    Vec2(float x, float y);
    Vec2(D2D1_POINT_2F p);

    D2D1_POINT_2F D2Point();
    D2D1_SIZE_F Size();
    Vec2 operator+(Vec2 &b);
    Vec2 operator+(Vec2 b);
    Vec2& operator+=(Vec2 &b);
    Vec2 operator-(Vec2 &b);
    Vec2& operator-=(Vec2 &b);

    Vec2 operator-();
    Vec2 operator/(float b);
};

class Transformation {
    public:
    Vec2 translation;
    float rotation;
    Transformation();
    D2D1::Matrix3x2F Matrix(Vec2 center);
};

class Rect {
    public:
    Transformation transform;
    ID2D1RectangleGeometry *geometry;
    Rect(Vec2 pos, Vec2 size, DXState *dx);

    D2D1_RECT_F Bound();
    D2D1_RECT_F OriginalBound();
    Vec2 Center();
    Vec2 OriginalCenter();
    D2D1::Matrix3x2F TransformMatrix();
};

class Text {
    public:
    Vec2 pos;
    std::string text;
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    Transformation transform;
    Text(Vec2 pos, std::string text, DXState *dx);

    float X();
    float Y();
};

class Circle {
    public:
    ID2D1EllipseGeometry *geometry;
    Transformation transform;
    Circle(Vec2 center, float radius, DXState *dx);

    D2D1_RECT_F Bound();
    D2D1_RECT_F OriginalBound();
    Vec2 Center();
    Vec2 OriginalCenter();
    D2D1::Matrix3x2F TransformMatrix();
};

constexpr float kPathCommandMove   = (float) 'M';
constexpr float kPathCommandCubic  = (float) 'C';
constexpr float kPathCommandLine   = (float) 'L';

class Path {
    public:
    Transformation transform;
    std::vector<float> commands;
    ID2D1PathGeometry *geometry;
    Path(std::vector<float> commands, DXState *dx);

    D2D1_RECT_F Bound();
    D2D1_RECT_F OriginalBound();
    Vec2 Center();
    Vec2 OriginalCenter();
    D2D1::Matrix3x2F TransformMatrix();
};

enum class ShapeType {
    None,
    Rect,
    Text,
    Circle,
    Path,
};

class ActiveShape {
    public:
    ShapeType type;
    int index;
    ActiveShape(ShapeType type, int index) : type(type), index(index) {};
    ActiveShape() : type(ShapeType::None), index(-1) {};
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
    D2D1::Matrix3x2F ScaleMatrix();
    D2D1::Matrix3x2F TranslationMatrix();
    D2D1::Matrix3x2F DocumentToScreenMat();
    D2D1::Matrix3x2F ScreenToDocumentMat();
};

class Document {
    public:
    std::vector<Rect> shapes;
    std::vector<Text> texts;
    std::vector<Circle> circles;
    std::vector<Path> paths;
    ActiveShape active_shape;
    Document() {};

    // TODO: check Hresult on calls to ContainsPoint
    void Click(Vec2 screen_pos, View *view) {
        D2D1_POINT_2F point = screen_pos.D2Point();


        bool clicked_shape = false;

        clicked_shape = this->CollectionHitTest(&this->shapes, point, view, ShapeType::Rect);
        if (clicked_shape) return;

        clicked_shape = this->CollectionHitTest(&this->circles, point, view, ShapeType::Circle);
        if (clicked_shape) return;

        clicked_shape = this->CollectionHitTest(&this->paths, point, view, ShapeType::Path);
        if (clicked_shape) return;

        this->active_shape = ActiveShape();
    }

    template <typename T>
    bool CollectionHitTest(std::vector<T> *collection, D2D1_POINT_2F point, View *view, ShapeType type) {
        D2D1::Matrix3x2F doc_to_screen = view->DocumentToScreenMat();

        for (auto i=0; i<collection->size(); i++) {
            T *shape = &(*collection)[i];

            BOOL contains_point;
            shape->geometry->StrokeContainsPoint(point, kHairline, NULL, shape->TransformMatrix() * doc_to_screen, &contains_point);

            if (contains_point) {
                printf("found you :)\n");
                this->active_shape = ActiveShape(type, i);
                return true;
            }
        }

        return false;
    }
};

class UIState {
    public:
    bool show_debug = false;
    bool show_command_prompt = false;
    bool show_demo_window = false;
};


class DXState {
    public:
    // Device Independent Direct2D resources
    ID2D1Factory* factory;
    IDWriteFactory *write_factory;
    IDWriteTextFormat *debug_text_format;

    // Device dependent Direct2D resources
    ID2D1RenderTarget* renderTarget;
    ID2D1SolidColorBrush* lightSlateGrayBrush;
    ID2D1SolidColorBrush* cornflowerBlueBrush;
    ID2D1SolidColorBrush* blackBrush;
    ID2D1SolidColorBrush* debugBrush;

    // Direct3D resources
    ID3D11Device* device;
    ID3D11DeviceContext* device_context;
    IDXGISwapChain* swap_chain;
    ID3D11RenderTargetView* render_target_view;

    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources(HWND hwnd);
    HRESULT CreateRenderTarget();
    HRESULT DiscardRenderTarget();
    HRESULT DiscardDeviceResources();
    void Teardown();
    HRESULT Resize(UINT width, UINT height);
    HRESULT Render(Document *doc, View *view, UIState *ui);
    HRESULT RenderRects(Document *doc, View *view);
    HRESULT RenderCircles(Document *doc, View *view);
    HRESULT RenderPaths(Document *doc, View *view);
    void RenderText(Document *doc, View *view);
    void RenderGridLines();
    void RenderDemoWindow(UIState *ui);
    void RenderDebugWindow(UIState *ui, View *view);
    void RenderCommandPrompt(UIState *ui, Document *doc, View *view);
    void RenderActiveSelectionWindow(Document *doc);
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateDebugConsole();
void CreateGuiContext();
void TeardownGui();
void ExitOnFailure(HRESULT hr);

#endif