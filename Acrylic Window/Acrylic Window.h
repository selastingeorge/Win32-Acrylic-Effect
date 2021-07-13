#pragma once

#include <windows.h>
#include <memory>
#include "AcrylicCompositor.h"

bool active;
std::unique_ptr<AcrylicCompositor> compositor{nullptr};
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);