#ifndef SVIGGY_H
#define SVIGGY_H

#include <d3d11.h>
#include <dxgi.h>
#include <dwrite.h>
#include <d2d1.h>
#include <string>
#include <unordered_map>

#include <system_error>

#include "ds.hpp"

#define RETURN_FAIL(hr) if(FAILED(hr)) return hr
// Subtract 1 from array size to avoid the null terminating character for b
#define STRNCMP(a, b) strncmp(a, b, ARRAYSIZE(b) - 1) == 0

constexpr float kPixelsPerInch = 50;
constexpr float kScaleDelta = 0.1;
constexpr float kTranslationDelta = 1;
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
    Vec2 scale;
    float rotation;
    Transformation();
    D2D1::Matrix3x2F Matrix(Vec2 center);
};

class Text {
    public:
    Vec2 pos;
    String text;
    IDWriteTextLayout *layout;
    IDWriteTextFormat *format;
    Transformation transform;
    Text(Vec2 pos, String text, DXState *dx);

    float X();
    float Y();
};

constexpr float kPathCommandMove   = (float) 'M';
constexpr float kPathCommandCubic  = (float) 'C';
constexpr float kPathCommandLine   = (float) 'L';
constexpr float kPathCommandArc    = (float) 'A';

constexpr float kClockwise = 1.0f;
constexpr float kCounterClockwise = 0.0f;

class Path {
    public:
    Transformation transform;
    ID2D1PathGeometry *geometry;
    size_t collection;
    DynamicArray<String> tags;
    Path(ID2D1PathGeometry *geometry);

    void Free() {
        this->tags.Free();
        this->geometry->Release();
    }

    static Path CreateLine(Vec2 from, Vec2 to, DXState *dx);
    static Path CreateRect(Vec2 pos, Vec2 size, DXState *dx);
    static Path CreateCircle(Vec2 center, float radius, DXState *dx);

    D2D1_RECT_F Bound();
    D2D1_RECT_F OriginalBound();
    Vec2 Center();
    Vec2 OriginalCenter();
    D2D1::Matrix3x2F TransformMatrix();
};

class PathBuilder {
    public:
    ID2D1GeometrySink *geometry_sink;
    ID2D1PathGeometry *geometry;
    bool has_open_figure;
    PathBuilder(DXState *dx);

    void Move(Vec2 to);
    void Line(Vec2 to);
    void Cubic(Vec2 c1, Vec2 c2, Vec2 end);
    void Arc(Vec2 end, Vec2 size, float rot, D2D1_SWEEP_DIRECTION direction);
    void Close();
    Path Build();
};

enum class ShapeType {
    None,
    Text,
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
    void ScrollZoom(bool in);
    D2D1::Matrix3x2F ScaleMatrix();
    D2D1::Matrix3x2F TranslationMatrix();
    D2D1::Matrix3x2F DocumentToScreenMat();
    D2D1::Matrix3x2F ScreenToDocumentMat();
};

class Document {
    public:
    DynamicArray<Text> texts;
    DynamicArray<Path> paths;
    DynamicArray<ActiveShape> active_shapes;
    View view;
    size_t next_collection = 0;
    Document();

    void Free() {
        this->texts.Free();
        this->paths.Free();
        this->active_shapes.Free();
    }

    void AddNewPath(Path p);
    size_t NextCollection();
    void SelectShape(Vec2 screen_pos);
    void SelectShapes(Vec2 start, Vec2 end);
    void TranslateView(Vec2 amount);
    void ScrollZoom(bool in);
    Vec2 MousePos();

    // Pipeline methods
    void RunPipeline();
    std::unordered_map<size_t, DynamicArray<size_t>> Collections();
};

class Application {
    public:
    DynamicArray<Document> documents;
    size_t active_doc;
    Application();

    Document* ActiveDoc();
    View* ActiveView();
    void ActivateDoc(size_t index);
};

class UIState {
    public:
    bool show_debug = false;
    bool show_command_prompt = false;
    bool show_demo_window = false;
    bool is_selecting = false;
    Vec2 selection_start = Vec2(0.0f, 0.0f);
};


class DXState {
    public:
    // Device Independent Direct2D resources
    ID2D1Factory* factory;
    IDWriteFactory *write_factory;
    IDWriteTextFormat *debug_text_format;
    ID2D1StrokeStyle *selection_stroke;

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
    HRESULT Render(Document *doc,  UIState *ui);
    HRESULT RenderRects(Document *doc);
    HRESULT RenderCircles(Document *doc);
    HRESULT RenderPaths(Document *doc);
    void RenderText(Document *doc);
    void RenderGridLines();
    void RenderDemoWindow(UIState *ui);
    void RenderDebugWindow(UIState *ui, Document *doc);
    void RenderCommandPrompt(UIState *ui, Document *doc);
    void RenderActiveSelectionWindow(Document *doc);
    void RenderActiveSelectionBox(Document *doc, UIState *ui);
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateDebugConsole();
void CreateGuiContext();
void TeardownGui();
void ExitOnFailure(HRESULT hr);

#endif