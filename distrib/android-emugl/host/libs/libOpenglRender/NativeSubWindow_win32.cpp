/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "NativeSubWindow.h"
#include <stdio.h>

EGLNativeWindowType createSubWindow(FBNativeWindowType p_window,
                                    int x, int y,int width, int height){
    static const char className[] = "subWin";

    WNDCLASS wc = {};
    if (!GetClassInfo(GetModuleHandle(NULL), className, &wc)) {
        wc.style =  CS_OWNDC | CS_HREDRAW | CS_VREDRAW;// redraw if size changes
        wc.lpfnWndProc = &DefWindowProc;               // points to window procedure
        wc.cbWndExtra = sizeof(void*);                 // save extra window memory, to store VasWindow instance
        wc.lpszClassName = className;                  // name of window class

        RegisterClass(&wc);
    }

    printf("creating window %d %d %d %d\n",x,y,width,height);
    EGLNativeWindowType ret = CreateWindowEx(
                        WS_EX_NOPARENTNOTIFY,  // do not bother our parent window
                        className,
                        "sub",
                        WS_CHILD|WS_DISABLED,
                        x,y,width,height,
                        p_window,
                        NULL,
                        NULL,
                        NULL);
    ShowWindow(ret, SW_SHOW);
    return ret;
}

void destroySubWindow(EGLNativeWindowType win){
    PostMessage(win, WM_CLOSE, 0, 0);
}
