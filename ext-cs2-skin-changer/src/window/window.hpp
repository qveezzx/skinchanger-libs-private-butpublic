#pragma once
#include <Windows.h>
#include <dwmapi.h>
#include <string>
#include <iostream>
#include <thread>
#include "../ui_engine.hpp"

namespace overlay {
    inline HWND Window = nullptr;
    inline WNDCLASSEXW wcex{};

    inline bool ShouldQuit = false;
    inline int G_Width = GetSystemMetrics(SM_CXSCREEN);
    inline int G_Height = GetSystemMetrics(SM_CYSCREEN);
    inline BYTE GlobalAlpha = 255;
    inline int G_MonitorIdx = 0; // 0 = Primary, 1 = Next, etc.

    inline HDC hdcScreen = nullptr;
    inline HDC hdcMem = nullptr;
    inline HBITMAP hbmMem = nullptr;
    inline HBITMAP hbmOld = nullptr;
    inline void* pBits = nullptr;

    inline void CreateLayeredWindow() {
        wcex = {
            sizeof(WNDCLASSEXW), 0, DefWindowProcW, 0, 0, nullptr,
            LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW),
            nullptr, nullptr, L"CS2_Overlay", LoadIcon(nullptr, IDI_APPLICATION)
        };

        RegisterClassExW(&wcex);

        // Monitor detection
        struct MonitorData { int target; int current; RECT rect; };
        MonitorData data = { G_MonitorIdx, 0, {0, 0, G_Width, G_Height} };
        
        EnumDisplayMonitors(NULL, NULL, [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
            MonitorData* d = (MonitorData*)dwData;
            if (d->current == d->target) {
                d->rect = *lprcMonitor;
            }
            d->current++;
            return TRUE;
        }, (LPARAM)&data);

        int winX = data.rect.left;
        int winY = data.rect.top;
        int winW = data.rect.right - data.rect.left;
        int winH = data.rect.bottom - data.rect.top;

        // Update globals for GUI mapping
        G_Width = winW;
        G_Height = winH;

        Window = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
            L"CS2_Overlay", L"Overlay",
            WS_POPUP, winX, winY, winW, winH,
            nullptr, nullptr, wcex.hInstance, nullptr
        );

        // NOTE: Do NOT call SetLayeredWindowAttributes if we plan to use UpdateLayeredWindow!
        // SetLayeredWindowAttributes(Window, 0, 255, LWA_ALPHA); 

        ShowWindow(Window, SW_SHOW);
        
        // DWM Blur (Optional, looks nice)
        MARGINS margin = { -1 };
        DwmExtendFrameIntoClientArea(Window, &margin);

        // Prep GDI DC
        hdcScreen = GetDC(nullptr); // Use Screen DC, not Window DC
        hdcMem = CreateCompatibleDC(hdcScreen);
        
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = G_Width;
        bmi.bmiHeader.biHeight = -G_Height; // Top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        hbmMem = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
        hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

        SC_GUI::Init();
    }

    void SetInput(bool enabled) {
        LONG style = GetWindowLong(Window, GWL_EXSTYLE);
        if (enabled) {
            style &= ~WS_EX_TRANSPARENT;
            SetFocus(Window);
        } else {
            style |= WS_EX_TRANSPARENT;
        }
        SetWindowLong(Window, GWL_EXSTYLE, style);
    }

    bool Setup() {
        CreateLayeredWindow();
        SetInput(true); // Default open
        return true;
    }

    void Render(void (*RenderCallback)(), bool visible) {
        SC_GUI::Input.scrollDelta = 0; // Reset per frame
        SC_GUI::Input.lastChar = 0;    // Reset char per frame

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            
            if (msg.message == WM_QUIT) ShouldQuit = true;
            if (msg.message == WM_MOUSEWHEEL) {
                SC_GUI::Input.scrollDelta = (short)HIWORD(msg.wParam);
            }
            if (msg.message == WM_CHAR) {
                 SC_GUI::Input.lastChar = (char)msg.wParam;
            }
        }

        if (ShouldQuit) return;

        // Clear Background (Transparent)
        memset(pBits, 0, G_Width * G_Height * 4);

        // Only draw if visible
        if (visible) {
            // Create Graphics from Bitmap in a scope to ensure destruction/flush
            {
                Gdiplus::Graphics g(hdcMem);
                SC_GUI::BeginFrame(&g, Window);
                RenderCallback();
            } // g renders here
        }

        // Update Layered Window
        BLENDFUNCTION blend = { AC_SRC_OVER, 0, GlobalAlpha, AC_SRC_ALPHA };
        POINT ptPos = { 0, 0 };
        SIZE size = { G_Width, G_Height };
        POINT ptSrc = { 0, 0 };

        UpdateLayeredWindow(Window, hdcScreen, &ptPos, &size, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);
    }
}