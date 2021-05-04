#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "user32")

#ifndef UNICODE
#define UNICODE
#endif

#include <d2d1.h>
#include <vector>
#include <windows.h>

#include "pugixml.hpp"

#include "svg.hpp"
#include "sviggy.hpp"

D2State d2state;
Document doc;
View view;

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
    LoadSVGFile((char *)"test-svg.svg", &doc);

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

        case WM_LBUTTONDOWN: {
            float screen_x = LOWORD(lParam);
            float screen_y = HIWORD(lParam);
            Position p = view.GetDocumentPosition(screen_x, screen_y);
            doc.shapes.emplace_back(p.x, p.y, 10, 10);
            return 0;
        }

        case WM_KEYDOWN: {
            switch (wParam) {
                case VK_LEFT:
                    view.x += 10;
                    break;

                case VK_RIGHT:
                    view.x -= 10;
                    break;

                case VK_DOWN:
                    view.y -= 10;
                    break;

                case VK_UP:
                    view.y += 10;
                    break;

                case VK_OEM_PLUS:
                    view.scale += 0.1;
                    break;

                case VK_OEM_MINUS:
                    view.scale -= 0.1;
                    break;

                default:
                    return 0;
            }
            return 0;
        }

        case WM_PAINT: {
            d2state.CreateDeviceResources(hwnd);
            d2state.Render(&doc, &view);
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