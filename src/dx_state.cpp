#include <d2d1.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "dxstate.hpp"
#include "sviggy.hpp"

HRESULT DXState::CreateDeviceIndependentResources() {
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
        L"Courier",
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

    return hr;
}


HRESULT DXState::CreateDeviceResources(HWND hwnd) {
    HRESULT hr = S_OK;
    // If we have a render target assume all of our device resoruces have been created
    if (renderTarget) {
        return hr;
    }

    RECT rc;
    GetClientRect(hwnd, &rc);

    D2D1_SIZE_U size = D2D1::SizeU(
        rc.right - rc.left,
        rc.bottom - rc.top
    );

    DXGI_SWAP_CHAIN_DESC sc_desc = {};
    sc_desc.BufferDesc.RefreshRate.Numerator   = 0;
    sc_desc.BufferDesc.RefreshRate.Denominator = 1;
    sc_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sc_desc.SampleDesc.Count  = 1;
    sc_desc.SampleDesc.Quality = 0;
    sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sc_desc.BufferCount = 1;
    sc_desc.OutputWindow = hwnd;
    sc_desc.Windowed = true;

    D3D_FEATURE_LEVEL feature_level;
    UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    hr = D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        flags,
        NULL,
        0,
        D3D11_SDK_VERSION,
        &sc_desc,
        &this->swap_chain,
        &this->device,
        &feature_level,
        &this->device_context
    );
    RETURN_FAIL(hr);

    // ImGui Device configuration stuff
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(this->device, this->device_context);

    hr = this->CreateRenderTarget();
    RETURN_FAIL(hr);

    hr = renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::LightSlateGray),
        &lightSlateGrayBrush
    );
    RETURN_FAIL(hr);

    hr = renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
        &cornflowerBlueBrush
    );
    RETURN_FAIL(hr);

    hr = renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Black),
        &blackBrush
    );
    RETURN_FAIL(hr);

    hr = renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Red),
        &debugBrush
    );

    return hr;
}

HRESULT DXState::CreateRenderTarget() {
    HRESULT hr = S_OK;

    IDXGISurface* back_buffer;
    hr = swap_chain->GetBuffer(
        0,
        IID_PPV_ARGS(&back_buffer)
    );

    if (FAILED(hr)) {
        return hr;
    }

    D2D1_RENDER_TARGET_PROPERTIES render_props = D2D1::RenderTargetProperties();
    render_props.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED);

    hr = this->factory->CreateDxgiSurfaceRenderTarget(
        back_buffer,
        render_props,
        &this->renderTarget
    );

    back_buffer->Release();

    if (FAILED(hr)) {
        return hr;
    }

    ID3D11Texture2D* buffer;
    this->swap_chain->GetBuffer(0, IID_PPV_ARGS(&buffer));
    this->device->CreateRenderTargetView(buffer, NULL, &this->render_target_view);
    buffer->Release();

    return hr;
}

HRESULT DXState::DiscardRenderTarget() {
    HRESULT hr = S_OK;
    if (this->render_target_view) {
        hr = this->render_target_view->Release();
        this->render_target_view = NULL;
    }

    if (FAILED(hr)) {
        return hr;
    }

    if (this->renderTarget) {
        hr = this->renderTarget->Release();
        this->renderTarget = NULL;
    }

    return hr;
}

HRESULT DXState::DiscardDeviceResources() {
    HRESULT hr = S_OK;

    hr = SafeRelease(&this->renderTarget);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->lightSlateGrayBrush);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->cornflowerBlueBrush);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->blackBrush);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->debugBrush);
    RETURN_FAIL(hr);

    hr = this->DiscardRenderTarget();
    RETURN_FAIL(hr);

    hr = this->device->Release();
    RETURN_FAIL(hr);

    hr = this->device_context->Release();
    RETURN_FAIL(hr);

    hr = this->swap_chain->Release();
    RETURN_FAIL(hr);

    hr = this->render_target_view->Release();
    RETURN_FAIL(hr);

    return hr;
}

void DXState::Teardown() {
    this->DiscardDeviceResources();

    SafeRelease(&this->debug_text_format);
    SafeRelease(&this->factory);
    SafeRelease(&this->write_factory);
}

HRESULT DXState::Resize(UINT width, UINT height) {
    HRESULT hr = S_OK;
    if (this->device) {
        hr = this->DiscardRenderTarget();
        RETURN_FAIL(hr);

        hr = this->swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        RETURN_FAIL(hr);

        hr = this->CreateRenderTarget();
        RETURN_FAIL(hr);
    }

    return hr;
}

HRESULT DXState::Render(Document *doc, View *view, UIState *ui) {
    HRESULT hr;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    this->RenderDemoWindow(ui);
    this->RenderDebugWindow(ui, view);
    this->RenderCommandPrompt(ui, doc, view);

    this->renderTarget->BeginDraw();
    this->renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    this->RenderGridLines();

    this->renderTarget->SetTransform(view->DocumentToScreenMat());

    D2D1_SIZE_F rtSize = this->renderTarget->GetSize();
    int width = static_cast<int>(rtSize.width);
    int height = static_cast<int>(rtSize.height);

    hr = this->RenderRects(doc, view);
    RETURN_FAIL(hr);

    hr = this->RenderCircles(doc, view);
    RETURN_FAIL(hr);

    hr = this->RenderPaths(doc, view);
    RETURN_FAIL(hr);

    this->RenderText(doc, view);

    hr = this->renderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        this->DiscardDeviceResources();
    } else {
        RETURN_FAIL(hr);
    }

    ImGui::Render();
    this->device_context->OMSetRenderTargets(1, &this->render_target_view, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    hr = this->swap_chain->Present(1, 0);

    return hr;
}

HRESULT DXState::RenderRects(Document *doc, View *view) {
    HRESULT hr = S_OK;

    for (auto &shape : doc->shapes) {
        hr = RenderShape(this, &shape);
        RETURN_FAIL(hr);
    }

    return hr;
}

HRESULT DXState::RenderCircles(Document *doc, View *view) {
    HRESULT hr = S_OK;

    for (auto &circle : doc->circles) {
        hr = RenderShape(this, &circle);
        RETURN_FAIL(hr);
    }

    return hr;
}

HRESULT DXState::RenderPaths(Document *doc, View *view) {
    HRESULT hr = S_OK;

    for (auto &path : doc->paths) {
        hr = RenderShape(this, &path);
        RETURN_FAIL(hr);
    }

    return hr;
}

void DXState::RenderText(Document *doc, View *view) {
    for (auto &text : doc->texts) {
        // So we could set the transform here but then we lose the original one
        // Maybe we create a new one, badda bing batta boom it then do our thing
        this->renderTarget->DrawTextLayout(text.pos.D2Point(), text.layout, this->blackBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
    }
}

void DXState::RenderGridLines() {
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

void DXState::RenderDemoWindow(UIState *ui) {
    if (!ui->show_demo_window) return;

    ImGui::ShowDemoWindow();
}

void DXState::RenderDebugWindow(UIState *ui, View *view) {
    if (!ui->show_debug) return;

    ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin("Debug");
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Text("Mouse Screen: (%.3f, %.3f)", view->mouse_pos_screen.x, view->mouse_pos_screen.y);
    ImGui::Text("Mouse Document: (%.3f, %.3f)", view->MousePos().x, view->MousePos().y);
    ImGui::End();
}

static char cmd_buf[256] = {0};
static bool was_visible_previous_frame = false;
ImGuiInputTextFlags cmd_flags = ImGuiInputTextFlags_EnterReturnsTrue;

void ClearCmdBuf() {
    char *iter = &cmd_buf[0];
    while (*iter) {
        *iter = NULL;
        iter++;
    }
}

void CommandPromptReset(UIState *ui) {
    ClearCmdBuf();
    ui->show_command_prompt = false;
}

char cmd_rect[] = "rect";
char cmd_view[] = "view";
char cmd_zoom[] = "zoom";

void DXState::RenderCommandPrompt(UIState *ui, Document *doc, View *view) {
    if (!ui->show_command_prompt) {
        was_visible_previous_frame = false;
        return;
    }


    ImGui::Begin("Command Prompt", NULL, ImGuiWindowFlags_NoTitleBar);

    if (!was_visible_previous_frame) {
        ImGui::SetKeyboardFocusHere();
        was_visible_previous_frame = true;
    }

    if (ImGui::InputText("", cmd_buf, ARRAYSIZE(cmd_buf), cmd_flags)) {
        if (STRNCMP(cmd_buf, cmd_rect)) {
            char *iter = &cmd_buf[ARRAYSIZE(cmd_rect)];

            Vec2 size = ParseVec(&iter);
            size.x = size.x ? size.x : 10.0f;
            size.y = size.y ? size.y : 10.0f;

            Vec2 pos = ParseVec(&iter);

            doc->shapes.emplace_back(pos, size, this);

            CommandPromptReset(ui);
            goto CmdPromptEnd;
        }

        if (STRNCMP(cmd_buf, cmd_view)) {
            char *iter = &cmd_buf[ARRAYSIZE(cmd_view)];

            Vec2 pos = ParseVec(&iter);
            view->start = pos;

            CommandPromptReset(ui);
            goto CmdPromptEnd;
        }

        if (STRNCMP(cmd_buf, cmd_zoom)) {
            char *iter = &cmd_buf[ARRAYSIZE(cmd_zoom)];

            float scale = std::strtof(iter, &iter);
            scale = scale ? scale : 1.0f;
            view->scale = scale;

            CommandPromptReset(ui);
            goto CmdPromptEnd;
        }
    }

    CmdPromptEnd:;
    ImGui::End();
}

Vec2 ParseVec(char **iter) {
    float x = std::strtof(*iter, iter);
    (*iter)++; // Advancing the iter allows for a comma inbetween
    float y = std::strtof(*iter, iter);

    return Vec2(x, y);
}