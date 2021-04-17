#ifndef SVIGGY_H
#define SVIGGY_H

#include <d2d1.h>
#include <vector>

class Rect {
    public:
    float x, y, width, height;
    Rect(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {};
};

class SviggyState {
    public:
    std::vector<Rect> shapes;
    float view_x = 0.0, view_y = 0.0, scale = 1.0;
    SviggyState() {};
};

class D2State {
    public:
    ID2D1Factory* factory;
    ID2D1HwndRenderTarget* renderTarget;
    ID2D1SolidColorBrush* lightSlateGrayBrush;
    ID2D1SolidColorBrush* cornflowerBlueBrush;
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

    HRESULT Render(SviggyState *state) {
        this->renderTarget->BeginDraw();

        D2D1::Matrix3x2F translation_matrix = D2D1::Matrix3x2F::Translation(state->view_x, state->view_y);
        D2D1::Matrix3x2F scale_matrix = D2D1::Matrix3x2F::Scale(state->scale, state->scale, D2D1_POINT_2F());
        this->renderTarget->SetTransform(scale_matrix * translation_matrix);
        this->renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

        D2D1_SIZE_F rtSize = this->renderTarget->GetSize();
        int width = static_cast<int>(rtSize.width);
        int height = static_cast<int>(rtSize.height);

        for (int x=0; x<width; x += 10) {
            this->renderTarget->DrawLine(
                D2D1::Point2F(static_cast<FLOAT>(x), 0.0f),
                D2D1::Point2F(static_cast<FLOAT>(x), rtSize.height),
                this->lightSlateGrayBrush,
                0.5f
            );
        }

        for (int y=0; y<height; y+= 10) {
            this->renderTarget->DrawLine(
                D2D1::Point2F(0.0,          static_cast<FLOAT>(y)),
                D2D1::Point2F(rtSize.width, static_cast<FLOAT>(y)),
                this->cornflowerBlueBrush,
                0.5f
            );
        }

        for (auto &shape : state->shapes) {
            D2D1_RECT_F rectangle = D2D1::RectF(
                shape.x, shape.y, shape.x + shape.width, shape.y + shape.height
            );
            this->renderTarget->DrawRectangle(&rectangle, this->lightSlateGrayBrush);
        }

        HRESULT hr = this->renderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            this->DiscardDeviceResources();
        }

        return hr;

    }
};

#endif