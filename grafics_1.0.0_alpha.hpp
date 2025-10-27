#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <iostream>
#include <thread>
#include <atomic>
using namespace std;
using namespace Gdiplus;
/*
 * Grafics.hpp , library to GUI in C++
 * Copyright (C) 2025 "MayTill"
 *
 * SPDX-License-Identifier: Apache 2.0
 *
 * This file is part of the project "Grafics-Library".
 * See LICENSE file for details.
 * If you don't get full file get it at 
 * https://github.com/MayTill/Grafics-library/blob/main/grafics_1.0.0_alpha.hpp
 */
// Funckje pomocnicze
void gcwh(string& a) {
    atomic<bool> inputReady(false);
    atomic<bool> quitFlag(false);

    thread inputThread([&]() {
        getline(cin, a);
        inputReady = true;
    });

    MSG msg;
    while (!inputReady && !quitFlag) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                quitFlag = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    inputThread.join();
    if (quitFlag) a.clear();
}

// --- obsługa komunikatów i opóźnienie ---
void MessHand(long long nanosecond) {
    MSG msg;
    auto start = chrono::steady_clock::now();

    while (true) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::nanoseconds>(now - start).count();
        if (elapsed >= nanosecond) break;

        this_thread::sleep_for(chrono::nanoseconds(1));
    }
}
void MessHandOnce() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return; // jeśli przyszło WM_QUIT, wyjdź
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static HWND g_hWnd = nullptr;
static ULONG_PTR gdiplusToken = 0;

// --- procedura okna ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// --- inicjalizacja okna ---
inline void iniWin(const string& name, int clientWidth = 800, int clientHeight = 600, bool borderless = false) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"Class";

    RegisterClass(&wc);

    DWORD style = borderless ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    DWORD exStyle = 0;

    RECT rc = {0,0, clientWidth, clientHeight};
    AdjustWindowRectEx(&rc, style, FALSE, exStyle);

    g_hWnd = CreateWindowEx(
        exStyle,
        wc.lpszClassName,
        wstring(name.begin(), name.end()).c_str(),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    if (!g_hWnd) {
        cerr << "Failed to create window!" << endl;
        return;
    }

    ShowWindow(g_hWnd, SW_SHOW);

    // GDI+ start
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
}

// --- ustawienie rozmiaru klienta ---
inline void SetWindowSize(int clientWidth, int clientHeight) {
    if (!g_hWnd) return;

    RECT rc = {0,0, clientWidth, clientHeight};
    DWORD style = GetWindowLong(g_hWnd, GWL_STYLE);
    DWORD exStyle = GetWindowLong(g_hWnd, GWL_EXSTYLE);

    AdjustWindowRectEx(&rc, style, FALSE, exStyle);

    MoveWindow(g_hWnd, 0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE);
}

// --- zamknięcie ---
inline void closeWin() {
    if (g_hWnd) DestroyWindow(g_hWnd);
    GdiplusShutdown(gdiplusToken);
}

// --- rysowanie pikseli ---
inline void SetPixelWin(int x, int y, COLORREF color) {
    if (!g_hWnd) return;
    HDC hdc = GetDC(g_hWnd);
    SetPixel(hdc, x, y, color);
    ReleaseDC(g_hWnd, hdc);
}

// --- rysowanie prostokąta ---
inline void DrawRectWin(int x, int y, int w, int h, COLORREF color) {
    if (!g_hWnd) return;
    HDC hdc = GetDC(g_hWnd);
    HBRUSH brush = CreateSolidBrush(color);
    RECT rect = { x, y, x + w, y + h };
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
    ReleaseDC(g_hWnd, hdc);
}

// --- rysowanie obrazu ---
inline void DrawImageWin(const char* filename, int x, int y, int sizeX, int sizeY) {
    if (!g_hWnd) return;

    HDC hdc = GetDC(g_hWnd);
    Bitmap* bmp = Bitmap::FromFile(wstring(filename, filename + strlen(filename)).c_str());
    if (!bmp || bmp->GetLastStatus() != Ok) {
        delete bmp;
        ReleaseDC(g_hWnd, hdc);
        return;
    }

    Graphics graphics(hdc);
    graphics.SetPageUnit(UnitPixel);   // 1 jednostka = 1 piksel
    graphics.SetPageScale(1.0f);       // bez skalowania DPI
    graphics.DrawImage(bmp, x, y, sizeX, sizeY);

    delete bmp;
    ReleaseDC(g_hWnd, hdc);
}

// --- wWinMain w .hpp ---
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    extern int main1(); // funkcja użytkownika
    main1();

    MSG msg;
    while (true) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(1); // odciążenie CPU
    }

    return 0;
}


