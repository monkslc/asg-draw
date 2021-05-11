#pragma comment(lib, "d2d1")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32")

#ifndef UNICODE
#define UNICODE
#endif

#include <d2d1.h>
#include <vector>
#include <windows.h>
#include <windowsx.h>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "pugixml.hpp"

#include "svg.hpp"
#include "sviggy.hpp"

DXState dxstate;
Document doc;
UIState ui;
View view;

#define FLAGCMP(num, flag) num & flag

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    HRESULT hr;

    // TODO: only do this in debug mode
    CreateDebugConsole();

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

    ShowWindow(hwnd, nCmdShow);

    // This is here just for testing purposes so we have something to look at on load
    LoadSVGFile((char *)"test-svg.svg", &doc, &dxstate);

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

            hr = dxstate.Render(&doc, &view, &ui);
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
                Vec2 p = view.GetDocumentPosition(Vec2(screen_x, screen_y));

                doc.shapes.emplace_back(Vec2(p.x, p.y), Vec2(10, 10), &dxstate);
                return 0;
            }

            case WM_MOUSEWHEEL: {
                bool scroll_direction = GET_WHEEL_DELTA_WPARAM(wParam) > 0;
                view.Scale(scroll_direction);
                return 0;
            }

            case WM_MOUSEMOVE: {
                Vec2 original_mouse = view.MousePos();

                Vec2 new_mouse_screen = Vec2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                view.mouse_pos_screen = new_mouse_screen;

                Vec2 new_mouse = view.MousePos();

                // TODO: I think this should be RBUTTON but right click and drag doesn't work with
                // the macboook trackpad on bootcamp
                if (FLAGCMP(wParam, MK_LBUTTON)) {
                    Vec2 change = original_mouse - new_mouse;
                    view.start += change;
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
                    case VK_LEFT:
                        view.start.x -= kTranslationDelta;
                        break;

                    case VK_RIGHT:
                        view.start.x += kTranslationDelta;
                        break;

                    case VK_DOWN:
                        view.start.y += kTranslationDelta;
                        break;

                    case VK_UP:
                        view.start.y -= kTranslationDelta;
                        break;

                    case VK_OEM_PLUS:
                        view.Scale(true);
                        break;

                    case VK_OEM_MINUS:
                        view.Scale(false);
                        break;

                    case 'Y':
                        ui.show_demo_window = !ui.show_demo_window;
                        break;

                    case 'D':
                        ui.show_debug = !ui.show_debug;
                        break;

                    case VK_OEM_2:
                        ui.show_command_prompt = !ui.show_command_prompt;
                        break;

                    default:
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
        // printf("%s\n", std::system_category().message(hr).c_str());
        std::exit(1);
    }
}