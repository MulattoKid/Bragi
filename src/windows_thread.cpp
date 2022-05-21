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

#include "windows_thread.h"

#include <assert.h>
#include <stdio.h>

void ThreadCreate(LPTHREAD_START_ROUTINE thread_function, void* thread_function_data, wchar_t* thread_name, HANDLE* thread_handle)
{
    // Create thread
    HANDLE handle = CreateThread(NULL, 0, thread_function, thread_function_data, 0, NULL);
    if (thread_handle == NULL)
    {
        printf("ERROR(%s:%i): Failed to create thread\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    // Name thread
    HRESULT hres = SetThreadDescription(handle, (PCWSTR)thread_name);
    if (FAILED(hres))
    {
        printf("ERROR(%s:%i): Failed to name thread '%s'\n", __FILE__, __LINE__, thread_name);
        exit(EXIT_FAILURE);
    }

    *thread_handle = handle;
}

void ThreadDestroy(HANDLE thread, DWORD exit_code)
{
    assert(thread != NULL);
    
    BOOL res = TerminateThread(thread, exit_code);
    if (res == 0)
    {
        printf("ERROR(%s:%i): Failed to destroy thread\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}