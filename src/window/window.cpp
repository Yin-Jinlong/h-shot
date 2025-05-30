//
// Created by yjl on 2025/5/26.
//

#include "window.h"
#include "dwmapi.h"

#include <glad/glad.h>
#include <include/gpu/ganesh/gl/GrGLBackendSurface.h>

#include "../ui/view.h"

#include "../guard.h"

Window* ActiveWindow = nullptr;

ATOM Window::init() {
    static ATOM atom = 0;
    if (!atom) {
        WNDCLASS wc = {};
        wc.lpszClassName = TEXT(APP_NAME);
        wc.lpfnWndProc = winProc;
        wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;

        atom = RegisterClass(&wc);
        if (!atom) {
            ShowErrorExit(1);
        }
    }
    return atom;
}

// ReSharper disable once CppParameterMayBeConst
LRESULT Window::winProc(HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    if (msg == WM_CREATE) {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ActiveWindow));
        ActiveWindow->onCreate(hWnd);
    }
    const auto win = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    return win ? win->proc(msg, wParam, lParam) : DefWindowProc(hWnd, msg, wParam, lParam);
}

SkIRect GetMonitorRect(const HMONITOR& hMonitor) {
    MONITORINFOEX info;
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(hMonitor, &info);
    return {info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right, info.rcMonitor.bottom};
}

// ReSharper disable once CppParameterMayBeConst
BOOL OnEnumDisplay(HMONITOR hMonitor, HDC hdc, LPRECT lpRect, const LPARAM lParam) {
    const auto rect = reinterpret_cast<std::vector<SkIRect>*>(lParam);
    const auto monitorRect = GetMonitorRect(hMonitor);
    rect->push_back(monitorRect);
    return TRUE;
}

sk_sp<SkSurface> Window::createSurface(const int width, const int height) const {
    const auto target = GrBackendRenderTargets::MakeGL(width, height, 1, 8, _frame_buffer_info);
    return SkSurfaces::WrapBackendRenderTarget(
        _gr_context.get(),
        target,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        SkColorSpace::MakeSRGB(),
        nullptr);
}


std::vector<SkIRect> Window::getDesktopRects() {
    std::vector<SkIRect> list;
    EnumDisplayMonitors(nullptr, nullptr, OnEnumDisplay, reinterpret_cast<LPARAM>(&list));
    return list;
}

std::vector<WinInfo> GetChildrenWindow(HWND parent, int depth, int maxDepth);

// ReSharper disable once CppParameterMayBeConst
BOOL OnEnumWindows(HWND hWnd, const LPARAM lParam) {
    if (IsWindowVisible(hWnd)) {
        const auto list = reinterpret_cast<std::vector<WinInfo>*>(lParam);
        RECT rect;
        GetWindowRect(hWnd, &rect);
        if (!IsIconic(hWnd) && rect.right - rect.left > 1 && rect.bottom - rect.top > 2) {
            // DWM窗口
            if (const auto exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
                (exStyle & WS_EX_NOREDIRECTIONBITMAP) == WS_EX_NOREDIRECTIONBITMAP) {
                BOOL cloaked = true;
                DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
                if (cloaked)
                    return TRUE;
            }
            const WinInfo info = {
                .hwnd = hWnd, .rect = SkIRect::MakeLTRB(rect.left, rect.top, rect.right, rect.bottom)};
            list->push_back(info);
        }
    }
    return TRUE;
}

// ReSharper disable once CppParameterMayBeConst
std::vector<WinInfo> GetChildrenWindow(HWND parent, const int depth, const int maxDepth) { // NOLINT(*-no-recursion)
    std::vector<WinInfo> list;
    if (depth < maxDepth) {
        EnumChildWindows(parent, OnEnumWindows, reinterpret_cast<LPARAM>(&list));
        for (auto& info : list) {
            info.children = GetChildrenWindow(info.hwnd, depth + 1, maxDepth);
        }
    }
    return list;
}

std::vector<WinInfo> Window::getWindowRects() { return GetChildrenWindow(nullptr, 0, 5); }

SkIRect Window::getDesktopRect(const std::vector<SkIRect>& rects) {
    SkIRect rect;
    for (auto [fLeft, fTop, fRight, fBottom] : rects) {
        rect.fLeft = std::min(rect.fLeft, fLeft);
        rect.fTop = std::min(rect.fTop, fTop);
        rect.fRight = std::max(rect.fRight, fRight);
        rect.fBottom = std::max(rect.fBottom, fBottom);
    }
    return rect;
}

static void EnableVSync() {
    if (const auto wglSwapIntervalExt = reinterpret_cast<void (*)(int)>(wglGetProcAddress("wglSwapIntervalEXT")))
        wglSwapIntervalExt(1);
}

Window::Window(RootState* state, const int exStyle, const int width, const int height) : _state(state) {
    const auto atom = init();

    ActiveWindow = this;

    _h_wnd = CreateWindowEx(
        exStyle,
        reinterpret_cast<LPTSTR>(atom),
        TEXT(APP_NAME),
        WS_POPUP,
        0,
        0,
        width,
        height,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    _h_wnd || ShowError();

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 16;

    _hdc = GetWindowDC(_h_wnd);

    auto pixelFormat = ChoosePixelFormat(_hdc, &pfd);
    pixelFormat || ShowErrorExit(2);

    SetPixelFormat(_hdc, pixelFormat, &pfd) || ShowErrorExit(3);

    _h_gl_rc = wglCreateContext(_hdc);

    _h_gl_rc || ShowError();

    wglMakeCurrent(_hdc, _h_gl_rc) || ShowError();

    if (!gladLoadGL()) {
        ShowError(TEXT("Could not load GL"));
        exit(-1);
    }

    _gr_context = GrDirectContexts::MakeGL();

    if (!_gr_context) {
        ShowError(TEXT("Could not create GrDirectContext"));
        exit(-1);
    }

    GLint fboId = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fboId);
    _frame_buffer_info = {.fFBOID = static_cast<GLuint>(fboId), .fFormat = GL_RGBA8};

    EnableVSync();

    _surface = createSurface(1, 1);

    hide();
    _state->onCreated();
}

Window::~Window() {
    delete _view;

    _gr_context->releaseResourcesAndAbandonContext();
    _surface = nullptr;

    wglMakeCurrent(_hdc, nullptr);
    wglDeleteContext(_h_gl_rc);
    _h_gl_rc = nullptr;

    ReleaseDC(_h_wnd, _hdc);
    _hdc = nullptr;
}

void Window::onCreate(const HWND& hWnd) {
    _h_wnd = hWnd;
    _state->onCreate(this);
}

HWND Window::hWnd() const { return _h_wnd; }

HDC Window::hDc() const { return _hdc; }

sk_sp<GrDirectContext> Window::context() { return _gr_context; }

sk_sp<SkSurface> Window::surface() const { return _surface; }

LRESULT Window::proc(const UINT msg, const WPARAM wParam, const LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
        resize(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_SETFOCUS:
        _state->onFocus(true);
        break;
    case WM_KILLFOCUS:
        _state->onFocus(false);
        break;
    case WM_SIZING: {
        const auto rect = reinterpret_cast<RECT*>(lParam);
        resize(rect->right - rect->left, rect->bottom - rect->top);
        break;
    }
    case WM_MOVE:
        redraw();
        break;
    case WM_PAINT:
        PAINTSTRUCT ps;
        BeginPaint(hWnd(), &ps);
        render();
        EndPaint(hWnd(), &ps);
        break;
    case WM_MOUSEWHEEL:
        if (_view)
            _view->dispatchMouseWheel(
                SkIPoint::Make(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), HIWORD(wParam), LOWORD(wParam));
        break;
    case WM_MOUSEMOVE:
        if (_view)
            _view->dispatchMouseMove(SkIPoint::Make(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
        break;
    case WM_LBUTTONDOWN:
        if (_view)
            _view->dispatchMouseDown(SkIPoint::Make(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), MOUSE_LEFT);
        break;
    case WM_RBUTTONDOWN:
        if (_view)
            _view->dispatchMouseDown(SkIPoint::Make(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), MOUSE_RIGHT);
        break;
    case WM_LBUTTONUP:
        if (_view)
            _view->dispatchMouseUp(SkIPoint::Make(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), MOUSE_LEFT);
        break;
    case WM_RBUTTONUP:
        if (_view)
            _view->dispatchMouseUp(SkIPoint::Make(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), MOUSE_RIGHT);
        break;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        if (_view)
            _view->dispatchKeyDown(wParam);
        break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
        if (_view)
            _view->dispatchKeyUp(wParam);
        break;
    case WM_CLOSE:
        DestroyWindow(_h_wnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_HOTKEY:
        _state->onCustomMessage(wParam, msg);
        break;
    default:
        if (msg >= WM_USER) {
            _state->onCustomMessage(wParam, lParam);
            break;
        }
        return DefWindowProc(_h_wnd, msg, wParam, lParam);
    }
    return 0;
}

void Window::redraw() const { InvalidateRect(_h_wnd, nullptr, false); }

void Window::resize(const int width, const int height) {
    _surface = createSurface(std::max(1, width), std::max(1, height));
}

// ReSharper disable once CppParameterMayBeConst
void Window::setRect(const SkIRect& rect, HWND order) const {
    setRect(rect.x(), rect.y(), rect.width(), rect.height(), order, 0);
}

// ReSharper disable once CppParameterMayBeConst
void Window::setRect(const int x, const int y, const int cx, const int cy, HWND order, const UINT uFlags) const {
    SetWindowPos(_h_wnd, order, x, y, cx, cy, uFlags);
}

// ReSharper disable once CppParameterMayBeConst
void Window::setOrder(HWND order) const { SetWindowPos(_h_wnd, order, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); }

bool Window::isVisible() const { return IsWindowVisible(_h_wnd); }

void Window::render() {
    wglMakeCurrent(_hdc, _h_gl_rc);
    const auto canvas = _surface->getCanvas();

    delete _view;

    _view = _state->build();
    _view->calcSize(SkSize::Make(static_cast<f32>(_surface->width()), static_cast<f32>(_surface->height())));
    const auto id = canvas->save();
    _view->draw(*canvas);
    canvas->restoreToCount(id);

    _gr_context->flushAndSubmit();
    SwapBuffers(_hdc);
}

void Window::show() const { ShowWindow(_h_wnd, SW_SHOW); }

void Window::hide() {
    if (!IsWindowVisible(_h_wnd))
        return;
    ShowWindow(_h_wnd, SW_HIDE);
    _gr_context->freeGpuResources();
    delete _view;
    _view = nullptr;
}

void Window::destroy() const { DestroyWindow(_h_wnd); }
