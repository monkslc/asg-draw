#include "d2debug.hpp"
#include "sviggy.hpp"

HRESULT D2State::CreateDeviceIndependentResources() {
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

     hr = write_factory->CreateTextFormat(
        L"Arial",
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0,
        L"",
        &debug_text_format
    );

    if (FAILED(hr)) {
        return hr;
    }

    debug_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    debug_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

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


HRESULT D2State::CreateDeviceResources(HWND hwnd) {
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

// TODO: need to add some things to this list
void D2State::DiscardDeviceResources() {
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

void D2State::Resize(UINT width, UINT height) {
    if (this->renderTarget) {
        this->renderTarget->Resize(D2D1::SizeU(width, height));
    }
}

HRESULT D2State::Render(Document *doc, View *view) {
    this->debug_info.Update();

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
    this->RenderPaths(doc, view);
    this->RenderText(doc, view);

    this->geometry_sink->Close();
    this->renderTarget->DrawGeometry(this->geometry, this->blackBrush, kHairline);

    this->RenderDebugInfo();

    HRESULT hr = this->renderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        this->DiscardDeviceResources();
    } else if (FAILED(hr)) {
        printf("Drawing Failed%s\n", std::system_category().message(hr).c_str());
    }

    return hr;
}

void D2State::RenderRects(Document *doc, View *view) {
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

void D2State::RenderLines(Document *doc, View *view) {
    for (auto &line : doc->lines) {
        this->renderTarget->DrawLine(line.start.D2Point(), line.end.D2Point(), this->blackBrush, kHairline, NULL);
    }
}

void D2State::RenderPolygons(Document *doc, View *view) {
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
}

void D2State::RenderCircles(Document *doc, View *view) {
    for (auto &circle : doc->circles) {
        D2D1_ELLIPSE ellipse = D2D1::Ellipse(circle.center.D2Point(), circle.radius, circle.radius);
        this->renderTarget->DrawEllipse(ellipse, this->blackBrush, kHairline, nullptr);
    }
}

void D2State::RenderPaths(Document *doc, View *view) {
    this->geometry_sink->SetFillMode(D2D1_FILL_MODE_WINDING);
    for (auto &path : doc->paths) {
        this->geometry_sink->BeginFigure(Vec2(0.0f, 0.0f).D2Point(), D2D1_FIGURE_BEGIN_HOLLOW);
        auto i=0;
        while (i < path.commands.size()) {
            if (path.commands[i] == kPathCommandMove) {
                Vec2 pos = Vec2(path.commands[i+1], path.commands[i+2]);
                i+=3;

                this->geometry_sink->EndFigure(D2D1_FIGURE_END_OPEN);
                this->geometry_sink->BeginFigure(pos.D2Point(), D2D1_FIGURE_BEGIN_HOLLOW);
                continue;
            }

            if (path.commands[i] == kPathCommandCubic) {
                Vec2 c1  = Vec2(path.commands[i+1], path.commands[i+2]);
                Vec2 c2  = Vec2(path.commands[i+3], path.commands[i+4]);
                Vec2 end = Vec2(path.commands[i+5], path.commands[i+6]);
                i += 7;

                D2D1_BEZIER_SEGMENT bezier = D2D1::BezierSegment(c1.D2Point(), c2.D2Point(), end.D2Point());
                this->geometry_sink->AddBezier(bezier);
                continue;
            }

            if (path.commands[i] == kPathCommandLine) {
                Vec2 to = Vec2(path.commands[i+1], path.commands[i+2]);
                i += 3;

                this->geometry_sink->AddLine(to.D2Point());
                continue;
            }

            printf("Unrecognized path command at %d of %.9f :(\n", i, path.commands[i]);
            i++;
            continue;
        }

        this->geometry_sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }
}

void D2State::RenderText(Document *doc, View *view) {
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

void D2State::RenderGridLines() {
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

void D2State::RenderDebugInfo() {
    this->renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    std::wstring wide_string = std::to_wstring(this->debug_info.fps);
    this->renderTarget->DrawText(
        wide_string.c_str(),
        wide_string.size(),
        this->debug_text_format,
        D2D1::RectF(0.0, 0.0, 100, 100),
        this->blackBrush
    );
}