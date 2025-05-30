#pragma once

#include "pre.h"
#include "ui/state.h"
#include "window/window.h"

class CaptureState final : public RootState
{
    enum ActionType : i8;
    enum CursorType : i8;

    Window* _window = nullptr;
    HMENU _tray_menu = nullptr;

    enum TryMenuItemId
    {
        TMI_OPEN,
        TMI_EXIT,
    };

public:
    /**
     * 低消耗模式，效能模式
     */
    static void enableLowMode();

    /**
     * 正常模式
     */
    static void disableLowMode();

    enum HotKey
    {
        HOTKEY_SCREEN
    };

    enum ActionType : i8
    {
        ACTION_TYPE_WAIT_RECT = -2,
        ACTION_TYPE_RECT = -1,
        ACTION_TYPE_NONE = 0,
        ACTION_TYPE_MODIFY_RECT_1,
        ACTION_TYPE_MODIFY_RECT_2,
        ACTION_TYPE_MODIFY_RECT_3,
        ACTION_TYPE_MODIFY_RECT_4,
        ACTION_TYPE_MODIFY_RECT_5,
        ACTION_TYPE_MODIFY_RECT_6,
        ACTION_TYPE_MODIFY_RECT_7,
        ACTION_TYPE_MODIFY_RECT_8,
        ACTION_TYPE_MODIFY_RECT_9,
    };

    enum CursorType : i8
    {
        CURSOR_NULL = -1,
        CURSOR_NONE,
        CURSOR_SELECT,
        CURSOR_MOVE,
        CURSOR_DIAGONAL,
        CURSOR_DIAGONAL_BACK,
        CURSOR_HORIZONTAL,
        CURSOR_VERTICAL,
    };

    sk_sp<SkImage> backgroundImage;
    SkPaint backgroundPaint;

    sk_sp<SkImage> image;
    ActionType actionType;
    CursorType cursorType;
    SkIRect selectedRect;
    SkIPoint mousePos;
    SkIPoint lastMousePos;

    SkPaint paint;
    SkScalar dash[2]{};

    SkScalar dashOffset = 0;
    SkScalar dashStep = 10;
    SkScalar dashWidth = 2;

    i32 magnifierRange = 50;
    f32 magnifierSize = 100;

    SkIRect desktopRect;
    // 0,0 开始
    std::vector<SkIRect> screenRects;
    std::vector<WinInfo> windowInfos;

    int selectDepth = 5;

    explicit CaptureState();
    ~CaptureState() override;

    void onCreate(Window* window) override;

    void onCreated() override;

    Window* window() const;

    void capture();

    void setCursor() const;

    View* build() override;

    SkIRect dstRect() const;

    void redraw() const;

    void reset();

    void show();
    void hide();

    void onHotKey(HotKey key);

    void onTray(TryMenuItemId id);

    void onCustomMessage(WPARAM wParam, LPARAM lParam) override;

    void onFocus(bool focus) override;

    /**
     * 选择鼠标处Rect
     * @param maxDepth 搜索最大深度，0表示只顶级
     * @param translate 是否转换到程序的坐标
     * @return
     */
    WinInfo selectWindow(int maxDepth, bool translate = true) const;
};
