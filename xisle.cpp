/*
 * X-Isle - An XAML Islands integration test for Rufus UI
 * Copyright © 2022-2023 Pete Batard <pete@akeo.ie>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "resource.h"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.system.h>
#include <winrt/windows.ui.xaml.hosting.h>
#include <winrt/windows.ui.xaml.markup.h>
#include <winrt/windows.ui.xaml.controls.h>
#include <winrt/Windows.ui.xaml.media.h>
#include <winrt/Windows.ui.xaml.controls.primitives.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

using namespace std;
using namespace winrt;
using namespace winrt::Windows::UI::Xaml;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::UI::Xaml::Markup;
using namespace Windows::UI::Xaml::Controls::Primitives;

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

HWND _hWnd;
HWND _childhWnd;
HINSTANCE _hInstance;

DependencyObject GetUIElement(DependencyObject root, const std::wstring& sName)
{
    if (root == nullptr)
        return nullptr;
    const int nNbChildren = Media::VisualTreeHelper::GetChildrenCount(root);
    for (int i = 0; i < nNbChildren; i++) {
        DependencyObject childObject = Media::VisualTreeHelper::GetChild(root, i);
        auto fe = childObject.try_as<FrameworkElement>();
        if (childObject != nullptr && fe.Name() == sName) {
            return childObject;
        } else {
            DependencyObject childInSubtree = GetUIElement(childObject, sName);
            if (childInSubtree != nullptr) {
                return childInSubtree;
            }
        }
    }
    return nullptr;
}

void DBG(std::string str)
{
    OutputDebugStringA(str.c_str());
    OutputDebugStringA("\r\n");
}

void DBG(std::wstring str)
{
    OutputDebugStringW(str.c_str());
    OutputDebugStringA("\r\n");
}

void DBG(winrt::hstring str)
{
    OutputDebugStringW(str.c_str());
    OutputDebugStringA("\r\n");
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    _hInstance = hInstance;

    // The main window class name.
    const wchar_t szWindowClass[] = L"Xisle";
    WNDCLASSEX windowClass = { };

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = szWindowClass;
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    windowClass.hIconSm = LoadIcon(windowClass.hInstance, IDI_APPLICATION);

    if (RegisterClassEx(&windowClass) == NULL) {
        MessageBox(NULL, L"Windows registration failed!", L"Error", NULL);
        return 0;
    }

    _hWnd = CreateWindow(
        szWindowClass,
        L"XAML Islands ComboBox Test",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    if (_hWnd == NULL) {
        MessageBox(NULL, L"Call to CreateWindow failed!", L"Error", NULL);
        return 0;
    }

    // Begin XAML Island section.

    // The call to winrt::init_apartment initializes COM; by default, in a multithreaded apartment.
    winrt::init_apartment(apartment_type::single_threaded);

    // Initialize the XAML framework's core window for the current thread.
    WindowsXamlManager winxamlmanager = WindowsXamlManager::InitializeForCurrentThread();

    // This DesktopWindowXamlSource is the object that enables a non-UWP desktop application 
    // to host WinRT XAML controls in any UI element that is associated with a window handle (HWND).
    DesktopWindowXamlSource desktopSource;

    // Get handle to the core window.
    auto interop = desktopSource.as<IDesktopWindowXamlSourceNative>();

    // Parent the DesktopWindowXamlSource object to the current window.
    check_hresult(interop->AttachToWindow(_hWnd));

    // This HWND will be the window handler for the XAML Island: A child window that contains XAML.  
    HWND hWndXamlIsland = nullptr;

    // Get the new child window's HWND. 
    interop->get_WindowHandle(&hWndXamlIsland);

    // Update the XAML Island window size because initially it is 0,0.
    SetWindowPos(hWndXamlIsland, nullptr, 0, 0, 800, 400, SWP_SHOWWINDOW);

    // Create the XAML content.
    Windows::UI::Xaml::Controls::Grid xamlContainer;

    HRSRC xamlResource = ::FindResource(NULL, MAKEINTRESOURCE(IDR_XAML), RT_RCDATA);
    HGLOBAL xamlResourceData = ::LoadResource(NULL, xamlResource);
    char* xamlStr = (char*)::LockResource(xamlResourceData);
    // NB: Only works if there's a single <Grid> element
    char* xamlStart = strstr(xamlStr, "<Grid");
    char* xamlEnd = strstr(xamlStr, "</Grid>");
    xamlEnd = &xamlEnd[7];
    DBG(wstring(xamlStart, xamlEnd));
    winrt::hstring xaml(wstring(xamlStart, xamlEnd));
    ::FreeResource(xamlResourceData);

    auto content = Windows::UI::Xaml::Markup::XamlReader::Load(xaml);
    xamlContainer.Children().Append(content.as<winrt::Windows::UI::Xaml::UIElement>());

    Windows::UI::Xaml::Controls::ComboBox cb1 = GetUIElement(xamlContainer, L"cb1").as<Windows::UI::Xaml::Controls::ComboBox>();
    if (cb1 != nullptr) {
        cb1.Items().Append(winrt::box_value(L"Item 1"));
        cb1.Items().Append(winrt::box_value(L"Item 2"));
        cb1.SelectedIndex(0);
    }

    Windows::UI::Xaml::Controls::ComboBox cb2 = GetUIElement(xamlContainer, L"cb2").as<Windows::UI::Xaml::Controls::ComboBox>();
    if (cb2 != nullptr) {
        cb2.Items().Append(winrt::box_value(L"Yolo 1"));
        cb2.Items().Append(winrt::box_value(L"Yolo 2"));
        cb2.SelectedIndex(1);
    }

    xamlContainer.UpdateLayout();
    desktopSource.Content(xamlContainer);

    // End XAML Island section.

    ShowWindow(_hWnd, nCmdShow);
    UpdateWindow(_hWnd);

    //Message loop:
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT messageCode, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    wchar_t greeting[] = L"Xisle";
    RECT rcClient;

    switch (messageCode) {
    case WM_PAINT:
        if (hWnd == _hWnd) {
            hdc = BeginPaint(hWnd, &ps);
            TextOut(hdc, 300, 5, greeting, (int)wcslen(greeting));
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

        // Create main window
    case WM_CREATE:
        _childhWnd = CreateWindowEx(0, L"ChildWClass", NULL, WS_CHILD | WS_BORDER, 0, 0, 0, 0, hWnd, NULL, _hInstance, NULL);
        return 0;

        // Main window changed size
    case WM_SIZE:
        // Get the dimensions of the main window's client
        // area, and enumerate the child windows. Pass the
        // dimensions to the child windows during enumeration.
        GetClientRect(hWnd, &rcClient);
        MoveWindow(_childhWnd, 0, 0, 200, 250, TRUE);
        ShowWindow(_childhWnd, SW_SHOW);

        return 0;

        // Process other messages.

    default:
        return DefWindowProc(hWnd, messageCode, wParam, lParam);
        break;
    }

    return 0;
}
