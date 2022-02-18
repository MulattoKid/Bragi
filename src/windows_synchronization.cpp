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

#include "windows_synchronization.h"

#include <stdio.h>

void SyncLockMutex(HANDLE mutex, DWORD wait_time_ms, const char* calle_file, int calle_line_number)
{
    DWORD res = WaitForSingleObject(mutex, wait_time_ms);
    if (res != WAIT_OBJECT_0)
    {
        printf("%s:%i Failed to wait on Mutex\n", calle_file, calle_line_number);
        exit(EXIT_FAILURE);
    }
}

void SyncReleaseMutex(HANDLE mutex, const char* calle_file, int calle_line_number)
{
    BOOL res = ReleaseMutex(mutex);
    if (res == 0)
    {
        printf("%s:%i Failed to release Mutex\n", calle_file, calle_line_number);
        exit(EXIT_FAILURE);
    }
}

void SyncSetEvent(HANDLE event, const char* calle_file, int calle_line_number)
{
    BOOL res = SetEvent(event);
    if (res == 0)
    {
        printf("%s:%i Failed to set Eventr\n", calle_file, calle_line_number);
        exit(EXIT_FAILURE);
    }
}

void SyncWaitOnEvent(HANDLE even, DWORD wait_time_ms, const char* calle_file, int calle_line_number)
{
    DWORD res = WaitForSingleObject(even, wait_time_ms);
    if (res != WAIT_OBJECT_0)
    {
        printf("%s:%i Failed to wait on Event\n", calle_file, calle_line_number);
        exit(EXIT_FAILURE);
    }
}