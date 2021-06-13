#pragma once

#include <windows.h>
#include "AcrylicCompositor.h"

bool active;
AcrylicCompositor* compositor;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);