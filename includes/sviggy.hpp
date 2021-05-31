#ifndef SVIGGY_H
#define SVIGGY_H

#include <d3d11.h>
#include <dxgi.h>
#include <dwrite.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_2.h>
#include <string>
#include <unordered_map>

#include <system_error>

#include "ds.hpp"

#define RETURN_FAIL(hr) if(FAILED(hr)) return hr
// Subtract 1 from array size to avoid the null terminating character for b
#define STRNCMP(a, b) strncmp(a, b, ARRAYSIZE(b) - 1) == 0

constexpr float kInfinity = std::numeric_limits<float>::infinity();
constexpr float kNegInfinity = -kInfinity;

constexpr float kPixelsPerInch = 50;
constexpr float kScaleDelta = 0.15;
constexpr float kTranslationDelta = 1;
constexpr float kHairline = 0.03;

// forward declarations
class DXState;

class Vec2 {
    public:
    float x,y;
    Vec2(float x, float y);
    Vec2(D2D1_POINT_2F p);

    static Vec2 Min();
    static Vec2 Max();

    D2D1_POINT_2F D2Point();
    D2D1_SIZE_F Size();
    bool Fits(Vec2 other);

    Vec2 operator+(Vec2 &b);
    Vec2& operator+=(Vec2 &b);
    Vec2 operator-(Vec2 &b);
    Vec2& operator-=(Vec2 &b);

    Vec2 operator-();
    Vec2 operator/(float b);
};

class Vec2Many {
    public:
    Vec2 vec2;
    float quantity; // float is a quantity so we can represent infinity
    Vec2Many(Vec2 vec2, float quantity);
};

class Vec2Named {
    public:
    Vec2 vec2;
    size_t id;
    Vec2Named(Vec2 vec2, size_t id);
};

class Rect {
    public:
    Vec2 pos, size;
    Rect(Vec2 pos, Vec2 size);
    Rect(D2D1_RECT_F* rect);

    float Left();
    float Top();
    float Right();
    float Bottom();

    float Width();
    float Height();

    bool Contains(Rect *other);

    Rect Union(Rect* other);
    D2D1_RECT_F D2Rect();
};

class RectNamed {
    public:
    Rect rect;
    size_t id;
    RectNamed(Rect rect, size_t id);

    float Left();
    float Top();
    float Right();
    float Bottom();
};

// Sorts the rect by its position in the x direction and then the y direction.
// If two rects are in the same position then the larger rect will end up first.
// This ensures that a rect containing another one will appear first
template <typename T>
bool SortRectPositionXY(T& a, T& b) {
    if (a.Left() != b.Left()) {
        return a.Left() < b.Left();
    }

    if (a.Top() != b.Top()) {
        return a.Top() < b.Top();
    }

    if (a.Right() != b.Right()) {
        return a.Right() > b.Right();
    }

    return a.Bottom() > b.Bottom();
}

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

    void Free();

    float X();
    float Y();
};

constexpr float kPathCommandMove   = (float) 'M';
constexpr float kPathCommandCubic  = (float) 'C';
constexpr float kPathCommandLine   = (float) 'L';
constexpr float kPathCommandArc    = (float) 'A';

constexpr float kClockwise = 1.0f;
constexpr float kCounterClockwise = 0.0f;

typedef size_t PathId;
typedef size_t CollectionId;

class ShapeData {
    public:
    Transformation transform;
    ID2D1Geometry* geometry;
    ShapeData(ID2D1Geometry* geometry);

    D2D1_MATRIX_3X2_F TransformMatrix();
};

class PathBuilder {
    public:
    ID2D1GeometrySink *geometry_sink;
    ID2D1PathGeometry *geometry;
    bool has_open_figure;
    PathBuilder(DXState *dx);

    static ShapeData CreateLine(Vec2 from, Vec2 to, DXState *dx);
    static ShapeData CreateRect(Vec2 pos, Vec2 size, DXState *dx);
    static ShapeData CreateCircle(Vec2 center, float radius, DXState *dx);

    void Move(Vec2 to);
    void Line(Vec2 to);
    void Cubic(Vec2 c1, Vec2 c2, Vec2 end);
    void Arc(Vec2 end, Vec2 size, float rot, D2D1_SWEEP_DIRECTION direction);
    void Close();
    ShapeData BuildPath(DXState *dx);
};

class Paths {
    public:
    DynamicArray<ShapeData>                 shapes;
    DynamicArray<ID2D1TransformedGeometry*> transformed_geometries;
    DynamicArray<ID2D1GeometryRealization*> low_fidelities;
    HashMap<PathId, size_t>                 index; // maps path id to index in one of the above arrays
    DynamicArray<PathId>                    reverse_index; // maps an index in one of the above arrays to a path id
    PathId                                  next_id;
    Paths(size_t estimated_cap);

    void Free();

    PathId AddPath(ShapeData p);
    PathId NextId();

    size_t Length();

    void RealizeGeometry(DXState *dx, PathId id);
    void RealizeAllGeometry(DXState *dx);

    ShapeData* GetShapeData(PathId id);
    ID2D1TransformedGeometry** GetTransformedGeometry(PathId id);
    ID2D1GeometryRealization** GetLowFidelity(PathId id);
    Rect GetBounds(PathId id);
};

class Collections {
    public:
    HashMap<PathId, CollectionId> collections;
    HashMap<CollectionId, DynamicArray<PathId>> reverse_collections_index;
    CollectionId next_id;
    Collections(size_t estimated_shapes);

    void Free();

    CollectionId NextId();
    CollectionId CreateCollectionForShape(PathId shape_id);
    CollectionId GetCollectionId(PathId shape_id);

    void SetCollection(PathId shape_id, CollectionId collection_id);
};

// TODO: map tags to a number. It'll make it faster to compare and then we don't have to store the string twice
class Tags {
    public:
    HashMap<PathId, DynamicArray<String>> tags; // maps a shape id to all of its tags
    HashMap<String, DynamicArray<PathId>> reverse_tags; // maps a tag to every shape that contains that tag
    Tags(size_t estimated_shapes);

    void Free();

    DynamicArray<String>* GetTags(PathId shape_id);
    void AddTag(size_t shape_id, char* tag_cstr);
};

enum class ShapeType {
    None,
    Text,
    Path,
};

class ActiveShape {
    public:
    size_t id;
    ActiveShape(size_t id) : id(id) {};
};

class View {
    public:
    Vec2 start;
    Vec2 mouse_pos_screen;
    float scale = 1.0;
    bool show_pipeline;
    View();

    Vec2 GetDocumentPosition(Vec2 screen_pos);
    Vec2 MousePos();
    void ScrollZoom(bool in);
    D2D1::Matrix3x2F ScaleMatrix();
    D2D1::Matrix3x2F TranslationMatrix();
    D2D1::Matrix3x2F DocumentToScreenMat();
    D2D1::Matrix3x2F ScreenToDocumentMat();
};

class Shape {
    public:
    ID2D1TransformedGeometry* geometry;
    Transformation transform;
    Shape(ID2D1TransformedGeometry* geometry, Transformation transform);
};

// Forward declarion for the Document
class Application;

class Document {
    public:
    DynamicArray<Text> texts;

    Paths paths;
    Collections collections;
    Tags tags;

    DynamicArray<ActiveShape> active_shapes;

    // Geometries in here don't have to get freed because they will be the same geometries from the document
    DynamicArray<Shape> pipeline_shapes;

    View view;
    size_t next_id = 0;
    size_t next_collection = 0;
    Document(size_t estimated_shapes);

    void Free();

    void AddNewPath(ShapeData p);

    void SelectShape(Vec2 screen_pos);
    void SelectShapes(Vec2 start, Vec2 end);

    void TranslateView(Vec2 amount);
    void ScrollZoom(bool in);
    Vec2 MousePos();
    void TogglePipelineView();

    void CollectActiveShapes();

    void AutoCollect();
};

class Application {
    public:
    DynamicArray<Document> documents;
    size_t active_doc;
    Application();

    void Free();

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
    ID2D1Factory*      factory;
    IDWriteFactory*    write_factory;
    IDWriteTextFormat* debug_text_format;
    ID2D1StrokeStyle*  selection_stroke;

    // Device dependent Direct2D resources
    ID2D1Device*          d2_device;
    ID2D1DeviceContext1*  d2_device_context;
    ID2D1SolidColorBrush* lightSlateGrayBrush;
    ID2D1SolidColorBrush* cornflowerBlueBrush;
    ID2D1SolidColorBrush* blackBrush;
    ID2D1SolidColorBrush* debugBrush;


    // Direct3D resources
    ID3D11Device*           device;
    ID3D11DeviceContext*    device_context;
    IDXGISwapChain*         swap_chain;
    IDXGIDevice*            dxgi_device;

    // Swapchain resources
    IDXGISurface*           dxgi_surface;
    ID2D1Bitmap1*           bitmap_target;
    ID3D11RenderTargetView* render_target_view;
    ID3D11Texture2D*        buffer;


    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceResources(HWND hwnd);
    HRESULT CreateSwapchainResources();
    HRESULT DiscardSwapchainResources();
    HRESULT DiscardDeviceResources();
    void Teardown();
    HRESULT Resize(UINT width, UINT height);
    HRESULT Render(Document *doc,  UIState *ui);
    void RenderPaths(Document *doc);
    void RenderPathsLowFidelity(Document *doc);
    void RenderPathsHighFidelity(Document *doc);
    void RenderText(Document *doc);
    void RenderPipeline(Document *doc);
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

Vec2 GeometryCenter(ID2D1Geometry* geometry);
Transformation GetTranslationTo(Vec2 to, Rect* from);
Rect GetBounds(ID2D1TransformedGeometry* geo);
void CreateGeometryRealizations(ShapeData* shape, ID2D1TransformedGeometry** transformed_geometry, ID2D1GeometryRealization** low_fidelity, DXState *dx);

#endif