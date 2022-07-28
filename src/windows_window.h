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

#ifndef WINDOWS_WINDOW_H
#define WINDOWS_WINDOW_H

#include <windows.h>

typedef enum
{
    WINDOW_KEY_ACTION_PRESSED = 0,
    WINDOW_KEY_ACTION_RELEASED = 1,
    WINDOW_KEY_ACTION_CHAR = 2
} window_key_action;

typedef struct
{
    WPARAM            key;
    window_key_action action;
} window_key_event;

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void             WindowCreate(HMODULE* win_instance, HWND* window);
void             WindowTaskbarShow(HWND window);
void             WindowTaskbarHide(HWND window);
void             WindowFullscreenMode(HWND window);

#endif