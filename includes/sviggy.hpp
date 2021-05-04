#ifndef SVIGGY_H
#define SVIGGY_H

#include <dwrite.h>
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

class Text {
    public:
    float x, y;
    std::string text;
    Text(float x, float y, std::string text) : x(x), y(y), text(text) {};
};

class Document {
    public:
    std::vector<Rect> shapes;
    std::vector<Text> texts;
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
    IDWriteFactory *write_factory;
    IDWriteTextFormat *text_format;

    ID2D1HwndRenderTarget* renderTarget;
    ID2D1SolidColorBrush* lightSlateGrayBrush;
    ID2D1SolidColorBrush* cornflowerBlueBrush;
    ID2D1SolidColorBrush* blackBrush;
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

        this->RenderText(doc, view);

        HRESULT hr = this->renderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            this->DiscardDeviceResources();
        }

        return hr;

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
                D2D1::RectF(text.x, text.y, 100, 100),
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