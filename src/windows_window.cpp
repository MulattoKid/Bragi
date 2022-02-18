/**
 * Copyright (c) 2022 - 2022, Daniel Fedai Larsen.
 *
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright
 * notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "windows_window.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

bool window_running = false;
bool window_active = false;
window_key_event window_key_events[UINT8_MAX]; // Is reset every frame
uint8_t window_key_releases_index; // Is reset every frame

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res_lres = 0;

    switch (uMsg)
    {
        // WIN32 window is created.
        case WM_CREATE:
        {
            printf("WIN32 INFO: window created\n");

            RECT window_rect;
            if (GetWindowRect(hwnd, &window_rect) == 0)
            {
                printf("WIND32 ERROR: failed to get window rectangle\n");
                exit(EXIT_FAILURE);
            }
            printf("WIN32 INFO: window dimensions: %ix%i\n", window_rect.right - window_rect.left, window_rect.bottom - window_rect.top);

            RECT client_rect;
            if (GetClientRect(hwnd, &client_rect) == 0)
            {
                printf("WIND32 ERROR: failed to get client rectangle\n");
                exit(EXIT_FAILURE);
            }
            printf("WIN32 INFO: client dimensions: %ix%i\n", client_rect.right - client_rect.left, client_rect.bottom - client_rect.top);

            window_running = true;
        } break;

        // WIN32 window's size changed.
        // TODO(Daniel): why is the rectangle returned by GetWindowRect larger than my screen resolution when WS_MAXIMIZE is specified?
        case WM_SIZE:
        {
            printf("WIN32 INFO: window size changed\n");

            RECT window_rect;
            if (GetWindowRect(hwnd, &window_rect) == 0)
            {
                printf("WIND32 ERROR: failed to get window rectangle\n");
                exit(EXIT_FAILURE);
            }
            printf("WIN32 INFO: window dimensions: %ix%i\n", window_rect.right - window_rect.left, window_rect.bottom - window_rect.top);

            RECT client_rect;
            if (GetClientRect(hwnd, &client_rect) == 0)
            {
                printf("WIND32 ERROR: failed to get client rectangle\n");
                exit(EXIT_FAILURE);
            }
            printf("WIN32 INFO: client dimensions: %ix%i\n", client_rect.right - client_rect.left, client_rect.bottom - client_rect.top);
        } break;

        // Windows has determined that the WIN32 window has either become active, or deactivate.
        case WM_ACTIVATEAPP:
        {
            window_active = !window_active;
        } break;

        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_ESCAPE:
                {
                    window_running = false;
                } break;

                case VK_SHIFT:
                {
                    // Check that we haven't encountered too many key downs this frame
                    assert(window_key_releases_index < 256);
                    window_key_events[window_key_releases_index] = { wParam, WINDOW_KEY_ACTION_PRESSED };
                    window_key_releases_index++;
                } break;

                // Others are handled by WM_CHAR
                default: {} break;
            }
        } break;
        
        case WM_KEYUP:
        {
            if (wParam == VK_SHIFT)
            {
                // Check that we haven't encountered too many key downs this frame
                assert(window_key_releases_index < 256);
                window_key_events[window_key_releases_index] = { wParam, WINDOW_KEY_ACTION_RELEASED };
                window_key_releases_index++;
            }
        } break;

        case WM_CHAR:
        {
            // Check that we haven't encountered too many key downs this frame
            assert(window_key_releases_index < 256);
            window_key_events[window_key_releases_index] = { wParam, WINDOW_KEY_ACTION_CHAR };
            window_key_releases_index++;
        } break;

        case WM_LBUTTONDOWN:
        {
            // Lower 16 bits
            WORD cursor_x_position = lParam;
            // Upper 16 bits
            WORD cursor_y_position = lParam >> 16;
        } break;

        // WIN32 window is closed (e.g. X button is pressed or ALT+F4).
        case WM_CLOSE:
        {
            window_running = false;
        } break;

        // WIN32 window has been destroyed.
        case WM_DESTROY:
        {
            // TODO(Daniel): handle this as an error - recreate window?
            window_running = false;
        } break;

        default:
        {
            res_lres = DefWindowProc(hwnd, uMsg, wParam, lParam);
        } break;
    }
    return res_lres;
}

void WindowCreate(HMODULE* instance, HWND* window)
{
    // Result values
    DWORD res_dword;

    // Get instance
    HMODULE tmp_instance = GetModuleHandleA(NULL);
    assert(tmp_instance != NULL);
    // Register a class for our WIN32 window
    WNDCLASS window_class;
    // TODO (Daniel): do we really need CS_OWNDC | CS_HREDRAW | CS_VREDRAW ?
    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = &WndProc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = tmp_instance;
    window_class.hIcon = NULL;
    window_class.hCursor = NULL;
    window_class.hbrBackground = NULL;
    window_class.lpszMenuName = NULL;
    window_class.lpszClassName = "SoundPlayer";
    if (RegisterClass(&window_class) == 0)
    {
        DWORD failure = GetLastError();
        printf("Failed to register class with error code %d (decimal)\n", failure);
        exit(EXIT_FAILURE);
    }
    // WS_POPUP: hides title bar
    // WS_VISIBLE: window will be shown
    HWND tmp_window = CreateWindow(window_class.lpszClassName, "", WS_POPUP | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, tmp_instance, NULL);
    if (tmp_window == NULL)
    {
        res_dword = GetLastError();
        printf("WIN32 ERROR: failed to create window with error core %d (decimal)\n", res_dword);
        exit(EXIT_FAILURE);
    }
    WindowTaskbarShow(tmp_window);

    /*
    HDC window_HDC = GetDC(tmp_window);
    int refreshRate = GetDeviceCaps(window_HDC, VREFRESH);
    */

    // Hide cursor
    int cursor_count = ShowCursor(FALSE);
    assert(cursor_count == -1);

    *instance = tmp_instance;
    *window = tmp_window;
}

void WindowTaskbarShow(HWND window)
{
    MONITORINFO monitor_info;
    monitor_info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor_info);
    SetWindowPos(window, HWND_TOP,
                 monitor_info.rcWork.left,
                 monitor_info.rcWork.top,
                 monitor_info.rcWork.right - monitor_info.rcWork.left,
                 monitor_info.rcWork.bottom - monitor_info.rcWork.top,
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

void WindowTaskbarHide(HWND window)
{
    MONITORINFO monitor_info;
    monitor_info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor_info);
    SetWindowPos(window, HWND_TOP,
                 monitor_info.rcMonitor.left,
                 monitor_info.rcMonitor.top,
                 monitor_info.rcMonitor.right,
                 monitor_info.rcMonitor.bottom,
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

void WindowFullscreenMode(HWND window)
{
    // TODO: Support 
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-changedisplaysettingsa
    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_EXT_full_screen_exclusive.html
    assert(0);
}