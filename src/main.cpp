#include "capture_state.h"
#include "window/window.h"

#ifdef NDEBUG
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
#else
int wmain()
#endif
{
    auto mutex = CreateMutex(nullptr, false, TEXT("Global\\" APP_NAME));
    guard _mutex([mutex] { CloseHandle(mutex); });
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (const auto hwnd = FindWindowEx(nullptr, nullptr, TEXT(APP_NAME), TEXT(APP_NAME))) {
            SendMessage(hwnd, WM_USER, CaptureState::HotKey::HOTKEY_SCREEN, WM_HOTKEY);
        }
        return 0;
    }

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    CaptureState state;

    Window w(&state, WS_EX_TOPMOST, 1, 1);

    RegisterHotKey(w.hWnd(), CaptureState::HOTKEY_SCREEN, MOD_NOREPEAT, VK_SNAPSHOT) ||
        ShowErrorExit(TEXT("RegisterHotKey Failed!"));

    MSG msg = {};
    BOOL ret;
    while ((ret = GetMessage(&msg, nullptr, 0, 0))) {
        if (ret == -1) {
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
