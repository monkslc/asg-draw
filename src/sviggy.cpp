#pragma comment(lib, "d2d1")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32")

#ifndef UNICODE
#define UNICODE
#endif

#include <d2d1.h>
#include <fileapi.h>
#include <windows.h>
#include <windowsx.h>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "pugixml.hpp"

#include "pipeline.hpp"
#include "svg.hpp"
#include "sviggy.hpp"

DXState dxstate;
Application app;
UIState ui;

SysAllocator global_allocator = SysAllocator();

#define FLAGCMP(num, flag) num & flag

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    HRESULT hr;

    #ifdef DEBUG
    CreateDebugConsole();
    #endif

    CreateGuiContext();

    const wchar_t CLASS_NAME[]  = L"Sample Window Class";

    hr = dxstate.CreateDeviceIndependentResources();
    ExitOnFailure(hr);

    WNDCLASS wc = { };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Sviggy :)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, L"Failed to create window", L"", NULL);
        return 1;
    }

    ExitOnFailure(hr);

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    TeardownGui();

    // Teardown was taking too long. Leaving it here commented out though so its
    // documented why we don't release the dx resources before exiting.
    // dxstate.Teardown();

    return 0;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HRESULT hr;

    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;

    // Non keybaord and mouse events
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZE: {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            hr = dxstate.Resize(width, height);
            ExitOnFailure(hr);
            return 0;
        }

        case WM_PAINT: {
            hr = dxstate.CreateDeviceResources(hwnd);
            ExitOnFailure(hr);

            hr = dxstate.Render(app.ActiveDoc(), &ui);
            ExitOnFailure(hr);
            return 0;
        }
    }

    ImGuiIO &io = ImGui::GetIO();

    // Handle mouse events if ImGui doesn't want them
    if (!io.WantCaptureMouse) {
        switch (uMsg) {
            case WM_LBUTTONDOWN: {
                float screen_x = LOWORD(lParam);
                float screen_y = HIWORD(lParam);

                Vec2 screen_pos = Vec2(screen_x, screen_y);

                ui.is_selecting    = true;
                ui.selection_start = screen_pos;
                return 0;
            }

            case WM_LBUTTONUP: {
                ui.is_selecting = false;

                float screen_x = LOWORD(lParam);
                float screen_y = HIWORD(lParam);

                Vec2 selection_end = Vec2(screen_x, screen_y);

                Vec2 difference = selection_end - ui.selection_start;
                if (std::abs(difference.x) <= 3 && std::abs(difference.y) <= 3) {
                    app.ActiveDoc()->SelectShape(ui.selection_start);
                    return 0;
                }

                Vec2 doc_selection_start = app.ActiveDoc()->view.GetDocumentPosition(ui.selection_start);
                Vec2 doc_selection_end   = app.ActiveDoc()->view.GetDocumentPosition(selection_end);

                app.ActiveDoc()->SelectShapes(doc_selection_start, doc_selection_end);
                return 0;
            }

            case WM_MOUSEWHEEL: {
                bool scroll_direction = GET_WHEEL_DELTA_WPARAM(wParam) > 0;
                app.ActiveDoc()->ScrollZoom(scroll_direction);
                return 0;
            }

            case WM_MOUSEMOVE: {
                Vec2 original_mouse = app.ActiveDoc()->MousePos();

                Vec2 new_mouse_screen = Vec2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                app.ActiveDoc()->view.mouse_pos_screen = new_mouse_screen;

                Vec2 new_mouse = app.ActiveDoc()->MousePos();

                if (FLAGCMP(wParam, MK_RBUTTON)) {
                    Vec2 change = original_mouse - new_mouse;
                    app.ActiveDoc()->TranslateView(change);
                }

                return 0;
            }
        }
    }

    // Handle keybaord events if ImGui doesn't want them
    if (!io.WantCaptureKeyboard) {
        switch (uMsg) {
            case WM_KEYDOWN: {
                switch (wParam) {
                    case VK_LEFT: {
                        Vec2 amount = Vec2(-kTranslationDelta, 0.0f);
                        app.ActiveDoc()->TranslateView(amount);
                        break;
                    }

                    case VK_RIGHT: {
                        Vec2 amount = Vec2(kTranslationDelta, 0.0f);
                        app.ActiveDoc()->TranslateView(amount);
                        break;
                    }

                    case VK_DOWN: {
                        Vec2 amount = Vec2(0.0f, kTranslationDelta);
                        app.ActiveDoc()->TranslateView(amount);
                        break;
                    }

                    case VK_UP: {
                        Vec2 amount = Vec2(0.0f, -kTranslationDelta);
                        app.ActiveDoc()->TranslateView(amount);
                        break;
                    }

                    case VK_OEM_PLUS:
                        app.ActiveDoc()->ScrollZoom(true);
                        break;

                    case VK_OEM_MINUS:
                        app.ActiveDoc()->ScrollZoom(false);
                        break;

                    case 'Y':
                        ui.show_demo_window = !ui.show_demo_window;
                        break;

                    case 'D':
                        ui.show_debug = !ui.show_debug;
                        break;

                    case 'L': {
                        // TODO: clean up this mess
                        const wchar_t* file = L"test-svg.svg";
                        WIN32_FILE_ATTRIBUTE_DATA fad;
                        if(!GetFileAttributesEx(file, GetFileExInfoStandard, &fad)) {
                            std::exit(1);
                        }

                        size_t shape_estimation = fad.nFileSizeLow / 250;
                        app.documents.Push(Document(shape_estimation));
                        app.ActivateDoc(app.documents.Length() - 1);

                        LoadSVGFile((char *)"test-svg.svg", app.ActiveDoc(), &dxstate);
                        break;
                    }

                    case VK_OEM_2:
                        ui.show_command_prompt = !ui.show_command_prompt;
                        break;

                    case 'P': {
                        if (app.documents.Length() < 2) {
                            app.documents.Push(Document(100));
                        }

                        PipelineActions p;

                        size_t memory_estimation = app.ActiveDoc()->paths.Length() * 100;
                        LinearAllocatorPool allocator = LinearAllocatorPool(memory_estimation);

                        // Right now these are strings because we can't compare a String to a StringEx currently but hopefully
                        // we come up with something better in the future
                        String bound_tag = String((char*) "Bound");
                        String text_tag  = String((char*) "Text");

                        TagId bound_id = app.ActiveDoc()->tag_god.GetTagId(bound_tag);
                        TagId text_id  = app.ActiveDoc()->tag_god.GetTagId(text_tag);

                        bound_tag.Free();
                        text_tag.Free();

                        DynamicArrayEx<TagId, LinearAllocatorPool> filter_tags;
                        filter_tags.Push(bound_id, &allocator);
                        filter_tags.Push(text_id, &allocator);
                        p.actions.Push(PipelineAction::Filter(filter_tags), &allocator);

                        auto bins = DynamicArrayEx<Vec2Many, LinearAllocatorPool>();
                        bins.Push(Vec2Many(Vec2(48, 24), kInfinity), &allocator);

                        p.actions.Push(PipelineAction::Layout(bins), &allocator);

                        p.Run(app.ActiveDoc(), &dxstate, &allocator);

                        allocator.FreeAllocator();
                        break;
                    }

                    case 'V':
                        app.ActiveDoc()->TogglePipelineView();
                        break;

                    case 'Q':
                        app.ActiveDoc()->AutoCollect();
                        break;

                    default:
                        if (wParam >= '0' && wParam <= '9') {
                            app.ActivateDoc(wParam - '0');
                        }
                        return 0;
                }
                return 0;
            }
        }
    }


    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CreateDebugConsole() {
    AllocConsole();
    FILE *file;
    freopen_s(&file, "CON", "w", stdout);
}

void CreateGuiContext() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
}

void TeardownGui() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ExitOnFailure(HRESULT hr) {
    if (FAILED(hr)) {
        // TODO: display error message here or write it out to a file using something like:
        printf("%s\n", std::system_category().message(hr).c_str());
        std::exit(1);
    }
}