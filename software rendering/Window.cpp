#include "Window.h"

HWND getActiveWindow() {
	return GetActiveWindow();
}

void centerMouse(HWND hwnd, int width, int height) {
    POINT pt = { width / 2, height / 2 };

    ClientToScreen(hwnd, &pt);

    SetCursorPos(pt.x, pt.y);
}