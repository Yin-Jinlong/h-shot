#pragma once

#include "../pre.h"

#include "../ui/state.h"

#include <include/gpu/ganesh/gl/GrGLDirectContext.h>
#include <include/gpu/ganesh/gl/GrGLTypes.h>

struct WinInfo
{
    HWND hwnd;
    SkIRect rect;
    std::vector<WinInfo> children;
};

class Window
{
    static ATOM init();

    static LRESULT winProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


    HWND _h_wnd;
    HDC _hdc;
    HGLRC _h_gl_rc;

    GrGLFramebufferInfo _frame_buffer_info;

    sk_sp<SkSurface> _surface;
    sk_sp<GrDirectContext> _gr_context;

    RootState* _state;
    View* _view = nullptr;

    sk_sp<SkSurface> createSurface(int width, int height) const;

public:
    static std::vector<SkIRect> getDesktopRects();
    static std::vector<WinInfo> getWindowRects();
    static SkIRect getDesktopRect(const std::vector<SkIRect>& rects = getDesktopRects());

    explicit Window(RootState* state, int exStyle, int width, int height);
    ~Window();

    void onCreate(const HWND& hWnd);

    USE_RET HWND hWnd() const;

    USE_RET HDC hDc() const;

    sk_sp<GrDirectContext> context();

    USE_RET sk_sp<SkSurface> surface() const;

    LRESULT proc(UINT msg, WPARAM wParam, LPARAM lParam);

    void redraw() const;

    void resize(int width, int height);

    void setRect(const SkIRect& rect, HWND order = nullptr) const;

    void setRect(int x, int y, int cx, int cy, HWND order = nullptr, UINT uFlags = 0) const;

    void setOrder(HWND order) const;

    USE_RET bool isVisible() const;

    void render();

    void show() const;

    void hide();

    void destroy() const;
};
