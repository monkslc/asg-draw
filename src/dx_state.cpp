#include <d2d1.h>
#include <d2d1_2.h>
#include <d2d1_2helper.h>
#include <d2d1_1helper.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_2.h>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "dxstate.hpp"
#include "sviggy.hpp"

HRESULT DXState::CreateDeviceIndependentResources() {
    HRESULT hr;
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &this->factory);

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

    float dashes[] = {3.0f};
    hr = this->factory->CreateStrokeStyle(
        D2D1::StrokeStyleProperties(
            D2D1_CAP_STYLE_FLAT,
            D2D1_CAP_STYLE_FLAT,
            D2D1_CAP_STYLE_ROUND,
            D2D1_LINE_JOIN_MITER,
            10.0f,
            D2D1_DASH_STYLE_CUSTOM,
            0.0f
        ),
        dashes,
        ARRAYSIZE(dashes),
        &this->selection_stroke
    );

    return hr;
}


HRESULT DXState::CreateDeviceResources(HWND hwnd) {
    HRESULT hr = S_OK;
    // If we have a device assume all of our device resoruces have been created
    if (this->d2_device) {
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

    #ifdef DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

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


    hr = this->device->QueryInterface(IID_PPV_ARGS(&this->dxgi_device));
    RETURN_FAIL(hr);

    auto props = D2D1::CreationProperties(D2D1_THREADING_MODE_SINGLE_THREADED, D2D1_DEBUG_LEVEL_WARNING, D2D1_DEVICE_CONTEXT_OPTIONS_NONE);
    hr = ((ID2D1Factory1*)this->factory)->CreateDevice(this->dxgi_device, &this->d2_device);
    RETURN_FAIL(hr);

    hr = d2_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, (ID2D1DeviceContext**)&this->d2_device_context);
    RETURN_FAIL(hr);

    this->CreateSwapchainResources();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(this->device, this->device_context);

    hr = d2_device_context->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::LightSlateGray),
        &lightSlateGrayBrush
    );
    RETURN_FAIL(hr);

    hr = d2_device_context->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
        &cornflowerBlueBrush
    );
    RETURN_FAIL(hr);

    hr = d2_device_context->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Black),
        &blackBrush
    );
    RETURN_FAIL(hr);

    hr = d2_device_context->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Red),
        &debugBrush
    );

    return hr;
}

// Swapchain resources need to be recreated everytime the window resizes
HRESULT DXState::CreateSwapchainResources() {
    HRESULT hr = S_OK;

    hr = this->swap_chain->GetBuffer(0, IID_PPV_ARGS(&this->dxgi_surface));
    RETURN_FAIL(hr);

    float dpix, dpiy;
    this->d2_device_context->GetDpi(&dpix, &dpiy);
    D2D1_BITMAP_PROPERTIES1 bprops = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
        dpix, dpiy
    );

    hr = this->d2_device_context->CreateBitmapFromDxgiSurface(this->dxgi_surface, &bprops, &this->bitmap_target);
    RETURN_FAIL(hr);

    this->d2_device_context->SetTarget(this->bitmap_target);

    this->swap_chain->GetBuffer(0, IID_PPV_ARGS(&this->buffer));
    this->device->CreateRenderTargetView(this->buffer, NULL, &this->render_target_view);

    return hr;
}

HRESULT DXState::DiscardSwapchainResources() {
    HRESULT hr = S_OK;

    hr = SafeRelease(&this->dxgi_surface);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->bitmap_target);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->render_target_view);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->buffer);
    RETURN_FAIL(hr);

    this->d2_device_context->SetTarget(NULL);

    return hr;
}

HRESULT DXState::DiscardDeviceResources() {
    HRESULT hr = S_OK;

    hr = SafeRelease(&this->d2_device);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->d2_device_context);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->lightSlateGrayBrush);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->cornflowerBlueBrush);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->blackBrush);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->debugBrush);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->device);
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->dxgi_device);
    RETURN_FAIL(hr);

    hr = this->DiscardSwapchainResources();
    RETURN_FAIL(hr);

    hr = SafeRelease(&this->swap_chain);
    RETURN_FAIL(hr);

    return hr;
}

void DXState::Teardown() {
    this->DiscardDeviceResources();

    SafeRelease(&this->debug_text_format);
    SafeRelease(&this->factory);
    SafeRelease(&this->write_factory);
    SafeRelease(&this->selection_stroke);
}

HRESULT DXState::Resize(UINT width, UINT height) {
    HRESULT hr = S_OK;
    if (this->device) {
        hr = this->DiscardSwapchainResources();
        RETURN_FAIL(hr);

        hr = this->swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        RETURN_FAIL(hr);

        hr = this->CreateSwapchainResources();
        RETURN_FAIL(hr);
    }

    return hr;
}

HRESULT DXState::Render(Document *doc, UIState *ui) {
    HRESULT hr;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    this->d2_device_context->BeginDraw();
    this->d2_device_context->Clear(D2D1::ColorF(D2D1::ColorF::White));

    this->RenderDemoWindow(ui);
    this->RenderDebugWindow(ui, doc);
    this->RenderCommandPrompt(ui, doc);
    this->RenderActiveSelectionWindow(doc);
    this->RenderActiveSelectionBox(doc, ui);

    this->RenderGridLines();

    this->d2_device_context->SetTransform(doc->view.DocumentToScreenMat());

    D2D1_SIZE_F rtSize = this->d2_device_context->GetSize();
    int width = static_cast<int>(rtSize.width);
    int height = static_cast<int>(rtSize.height);

    if (doc->view.show_pipeline) {
        this->RenderPipeline(doc);
    } else {
        this->RenderPaths(doc);
        this->RenderText(doc);
    }

    hr = this->d2_device_context->EndDraw();
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

void DXState::RenderPaths(Document *doc) {
    HRESULT hr = S_OK;

    if (doc->view.scale < 0.1) {
        return this->RenderPathsLowFidelity(doc);
    }

    return this->RenderPathsHighFidelity(doc);
}

void DXState::RenderPathsLowFidelity(Document *doc) {
    ID2D1GeometryRealization** data = doc->paths.low_fidelities.Data();
    for (auto i=0; i<doc->paths.low_fidelities.Length(); i++) {
        this->d2_device_context->DrawGeometryRealization(*data, this->blackBrush);
        data++;
    }
}

void DXState::RenderPathsHighFidelity(Document *doc) {
    ID2D1TransformedGeometry** data = doc->paths.transformed_geometries.Data();
    for (auto i=0; i<doc->paths.transformed_geometries.Length(); i++) {
        this->d2_device_context->DrawGeometry(*data, this->blackBrush, kHairline);
        data++;
    }
}

void DXState::RenderText(Document *doc) {
    for (auto i=0; i<doc->texts.Length(); i++) {
        Text* text = doc->texts.GetPtr(i);

        this->d2_device_context->DrawTextLayout(text->pos.D2Point(), text->layout, this->blackBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
    }
}

void DXState::RenderPipeline(Document *doc) {
    for (auto i=0; i<doc->pipeline_shapes.Length(); i++) {
        Shape shape = doc->pipeline_shapes.Get(i);
        D2D1_RECT_F bound;
        HRESULT hr = shape.geometry->GetBounds(NULL, &bound);
        Vec2 center = Vec2((bound.left + bound.right) / 2, (bound.top + bound.bottom) / 2);

        ID2D1Geometry* og;
        ID2D1TransformedGeometry* geometry;
        this->factory->CreateTransformedGeometry(
            shape.geometry,
            shape.transform.Matrix(center),
            &geometry
        );

        this->d2_device_context->DrawGeometry(geometry, this->blackBrush, kHairline, NULL);

        geometry->Release();
    }
}

void DXState::RenderGridLines() {
    this->d2_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
    D2D1_SIZE_F size = d2_device_context->GetSize();

    int width  = static_cast<int>(size.width);
    int height = static_cast<int>(size.height);

    for (int x = 0; x < width; x += 10) {
        d2_device_context->DrawLine(
            D2D1::Point2F(static_cast<FLOAT>(x), 0.0f),
            D2D1::Point2F(static_cast<FLOAT>(x), size.height),
            cornflowerBlueBrush,
            0.1f
        );
    }

    for (int y = 0; y < height; y += 10) {
        d2_device_context->DrawLine(
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

void DXState::RenderDebugWindow(UIState *ui, Document *doc) {
    if (!ui->show_debug) return;

    ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin("Debug");
    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Text("%zu paths", doc->paths.Length());
    ImGui::Text("%zu texts", doc->texts.Length());
    ImGui::Text("Mouse Screen: (%.3f, %.3f)", doc->view.mouse_pos_screen.x, doc->view.mouse_pos_screen.y);
    ImGui::Text("Mouse Document: (%.3f, %.3f)", doc->MousePos().x, doc->MousePos().y);
    ImGui::Text("Scale: %.3f", doc->view.scale);
    ImGui::Text("View: (%.3f, %.3f)", doc->view.start.x, doc->view.start.y);
    ImGui::End();
}

static char cmd_buf[256] = {0};
static bool was_visible_previous_frame = false;
ImGuiInputTextFlags cmd_flags = ImGuiInputTextFlags_EnterReturnsTrue;

void ClearBuf(char *buf) {
    char *iter = buf;
    while (*iter) {
        *iter = NULL;
        iter++;
    }
}

void CommandPromptReset(UIState *ui) {
    ClearBuf(cmd_buf);
    ui->show_command_prompt = false;
}

char cmd_rect[] = "rect";
char cmd_view[] = "view";
char cmd_zoom[] = "zoom";

void DXState::RenderCommandPrompt(UIState *ui, Document *doc) {
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

            doc->AddNewPath(PathBuilder::CreateRect(pos, size, this));

            CommandPromptReset(ui);
            goto CmdPromptEnd;
        }

        if (STRNCMP(cmd_buf, cmd_view)) {
            char *iter = &cmd_buf[ARRAYSIZE(cmd_view)];

            Vec2 pos = ParseVec(&iter);
            doc->view.start = pos;

            CommandPromptReset(ui);
            goto CmdPromptEnd;
        }

        if (STRNCMP(cmd_buf, cmd_zoom)) {
            char *iter = &cmd_buf[ARRAYSIZE(cmd_zoom)];

            float scale = std::strtof(iter, &iter);
            scale = scale ? scale : 1.0f;
            doc->view.scale = scale;

            CommandPromptReset(ui);
            goto CmdPromptEnd;
        }
    }

    CmdPromptEnd:;
    ImGui::End();
}

char tag_buf[256] = {0};

void DXState::RenderActiveSelectionWindow(Document *doc) {
    if (doc->active_shapes.Length() == 0) return;

    ImGui::Begin("Active Selection");

    for (auto i=0; i<doc->active_shapes.Length(); i++) {
        ActiveShape *shape = doc->active_shapes.GetPtr(i);

        if (ImGui::TreeNode(shape, "Shape %zu\n", shape->id)) {
            ShapeData* shape_data     = doc->paths.GetShapeData(shape->id);
            Transformation* transform = &shape_data->transform;

            size_t collection = doc->collectionsz.GetCollectionId(shape->id);

            Rect bound = doc->paths.GetBounds(shape->id);

            ImGui::Text("Shape Id: %zu", shape->id);
            ImGui::Text("Collection: %zu", collection);

            ImGui::Text("Pos: (%.3f, %.3f)", bound.Left(), bound.Top());
            ImGui::Text("Size: (%.3f, %.3f)", bound.Width(), bound.Height());

            if(ImGui::DragFloat("Translation x", &transform->translation.x, 0.125)) doc->paths.RealizeGeometry(this, shape->id);
            if(ImGui::DragFloat("Translation y", &transform->translation.y, 0.125)) doc->paths.RealizeGeometry(this, shape->id);
            if(ImGui::DragFloat("Scale x",       &transform->scale.x,       0.125)) doc->paths.RealizeGeometry(this, shape->id);
            if(ImGui::DragFloat("Scale y",       &transform->scale.y,       0.125)) doc->paths.RealizeGeometry(this, shape->id);

            if(ImGui::SliderFloat("Rotation", &transform->rotation, 0.0f, 360.0f)) doc->paths.RealizeGeometry(this, shape->id);

            ImGui::Text("Tags:");
            DynamicArray<String>* tags = doc->tags.GetTags(shape->id);

            if (tags) {
                for (auto i=0; i<tags->Length(); i++) {
                    String *tag = tags->GetPtr(i);
                    ImGui::Text("%s", tag->CStr());
                }
            }

            if (ImGui::InputText("Add Tag", tag_buf, ARRAYSIZE(tag_buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                doc->tags.AddTag(shape->id, tag_buf);
                ClearBuf(tag_buf);
            }

            ImGui::TreePop();
        }
    }

    if(ImGui::Button("Collect")) {
        doc->CollectActiveShapes();
    }


    ImGui::End();
}

void DXState::RenderActiveSelectionBox(Document *doc, UIState *ui) {
    if (!ui->is_selecting) return;

    float left  = ui->selection_start.x;
    float top   = ui->selection_start.y;
    float right = doc->view.mouse_pos_screen.x;
    float bot   = doc->view.mouse_pos_screen.y;

    D2D1_RECT_F box = D2D1::RectF(left, top, right, bot);

    this->d2_device_context->SetTransform(D2D1::Matrix3x2F::Identity());
    this->d2_device_context->DrawRectangle(box, this->blackBrush, 1.0, this->selection_stroke);
}

Vec2 ParseVec(char **iter) {
    float x = std::strtof(*iter, iter);
    (*iter)++; // Advancing the iter allows for a comma inbetween
    float y = std::strtof(*iter, iter);

    return Vec2(x, y);
}