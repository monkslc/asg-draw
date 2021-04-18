#ifndef SVIGGY_H
#define SVIGGY_H

#include <d2d1.h>
#include <vector>

constexpr float kPixelsPerInch = 50;

struct Position {
    float x,y;
};

class Rect {
    public:
    float x, y, width, height;
    Rect(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {};
};

class Document {
    public:
    std::vector<Rect> shapes;
    Document() {};
};

class View {
    public:
    float x = 0.0, y = 0.0, scale = 1.0;
    View() {};
    Position GetDocumentPosition(float screen_position_x, float screen_position_y) {
        D2D1::Matrix3x2F screen_to_document = this->GetTransformationMatrix();
        screen_to_document.Invert();

        D2D1_POINT_2F doc_position = screen_to_document.TransformPoint(D2D1_POINT_2F {screen_position_x, screen_position_y});
        return Position {
            doc_position.x,
            doc_position.y,
        };
    }

    D2D1::Matrix3x2F GetTransformationMatrix() {
        D2D1::Matrix3x2F translation_matrix = D2D1::Matrix3x2F::Translation(this->x, this->y);
        D2D1::Matrix3x2F scale_matrix = D2D1::Matrix3x2F::Scale(
            this->scale * kPixelsPerInch,
            this->scale * kPixelsPerInch,
            D2D1_POINT_2F()
        );

        return scale_matrix * translation_matrix;

    }
};

class D2State {
    public:
    ID2D1Factory* factory;
    ID2D1HwndRenderTarget* renderTarget;
    ID2D1SolidColorBrush* lightSlateGrayBrush;
    ID2D1SolidColorBrush* cornflowerBlueBrush;
    ID2D1SolidColorBrush* blackBrush;
    HRESULT InitializeFactory() {
        return D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &this->factory);
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

        this->renderTarget->SetTransform(view->GetTransformationMatrix());

        D2D1_SIZE_F rtSize = this->renderTarget->GetSize();
        int width = static_cast<int>(rtSize.width);
        int height = static_cast<int>(rtSize.height);

        for (auto &shape : doc->shapes) {
            D2D1_RECT_F rectangle = D2D1::RectF(
                shape.x,
                shape.y,
                (shape.x + shape.width),
                (shape.y + shape.height)
            );
            this->renderTarget->DrawRectangle(&rectangle, this->blackBrush, 0.003);
        }


        HRESULT hr = this->renderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            this->DiscardDeviceResources();
        }

        return hr;

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

#endif