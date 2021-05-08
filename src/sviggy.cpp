#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "user32")

#ifndef UNICODE
#define UNICODE
#endif

#include <d2d1.h>
#include <vector>
#include <windows.h>
#include <windowsx.h>

#include "pugixml.hpp"

#include "svg.hpp"
#include "sviggy.hpp"

D2State d2state;
Document doc;
View view;

#define FLAGCMP(num, flag) num & flag

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // TODO: only do this in debug mode
    CreateDebugConsole();

    const wchar_t CLASS_NAME[]  = L"Sample Window Class";

    HRESULT hr = d2state.CreateDeviceIndependentResources();

    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to Create the D2D1 Device Independent Resources. Exiting program", L"Direct2D Error", 0);
        return 2;
    }

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
        return 2;
    }

    ShowWindow(hwnd, nCmdShow);

    // This is here just for testing purposes so we have something to look at on load
    LoadSVGFile((char *)"test-svg.svg", &doc, &d2state);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    d2state.DiscardDeviceResources();

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZE: {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            d2state.Resize(width, height);
            return 0;
        }

        case WM_PAINT: {
            d2state.CreateDeviceResources(hwnd);
            d2state.Render(&doc, &view);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            float screen_x = LOWORD(lParam);
            float screen_y = HIWORD(lParam);
            Vec2 p = view.GetDocumentPosition(Vec2(screen_x, screen_y));

            doc.shapes.emplace_back(Vec2(p.x, p.y), Vec2(10, 10), &d2state);
            return 0;
        }

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

                default:
                    return 0;
            }
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

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CreateDebugConsole() {
    AllocConsole();
    FILE *file;
    freopen_s(&file, "CON", "w", stdout);
}