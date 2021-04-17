#pragma comment(lib, "user32")
#pragma comment(lib, "d2d1")

#ifndef UNICODE
#define UNICODE
#endif

#include <d2d1.h>
#include <stdio.h>
#include <windows.h>
#include <vector>

#include "sviggy.hpp"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

D2State d2state;
SviggyState state;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const wchar_t CLASS_NAME[]  = L"Sample Window Class";

    HRESULT hr = d2state.InitializeFactory();

    if (FAILED(hr)) {
        MessageBox(NULL, L"Failed to Create the D2D1 Factory. Exiting program", L"Direct2D Error", 0);
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
            UINT x = LOWORD(lParam);
            UINT y = HIWORD(lParam);
            state.shapes.emplace_back(x, y, 25, 25);
            return 0;
        }

        case WM_KEYDOWN: {
            switch (wParam) {
                case VK_LEFT:
                    state.view_x -= 10;
                    break;

                case VK_RIGHT:
                    state.view_x += 10;
                    break;

                case VK_DOWN:
                    state.view_y += 10;
                    break;

                case VK_UP:
                    state.view_y -= 10;
                    break;

                case VK_OEM_PLUS:
                    state.scale += 0.1;
                    break;

                case VK_OEM_MINUS:
                    state.scale -= 0.1;
                    break;

                default:
                    return 0;
            }
            return 0;
        }

        case WM_PAINT: {
            d2state.CreateDeviceResources(hwnd);
            d2state.Render(&state);
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}