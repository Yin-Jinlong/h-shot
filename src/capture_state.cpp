//
// Created by yjl on 2025/5/27.
//

#include "capture_state.h"

#include "include/effects/SkDashPathEffect.h"
#include "include/effects/SkImageFilters.h"
#include "include/encode/SkPngEncoder.h"

#include <complex>

#include "ui/view.h"
#include "window/window.h"

bool HasContent(const SkIRect& rect) { return rect.width() != 0 || rect.height() != 0; }

class CaptureView final : public View
{
    SkPixmap _pixmap;

public:
    SkBitmap bitmap = {};
    CaptureState& state;

    explicit CaptureView(CaptureState* state) : View(state), state(*state) {}

    void drawRect(SkCanvas& canvas, const SkIRect& rect) const {
        const auto sw2 = state.dashWidth / 2;
        state.paint.setStrokeWidth(state.dashWidth);
        state.paint.setStyle(SkPaint::kStroke_Style);

        const auto path = SkPath::Rect(
            SkRect::MakeLTRB(
                static_cast<f32>(rect.fLeft) + sw2,
                static_cast<f32>(rect.fTop) + sw2,
                static_cast<f32>(rect.fRight) - sw2 + 1,
                static_cast<f32>(rect.fBottom) - sw2 + 1));

        state.paint.setColor4f(SkColor4f{1, 1, 1, 0.6});
        state.paint.setPathEffect(SkDashPathEffect::Make(state.dash, 2, state.dashOffset));
        canvas.drawPath(path, state.paint);

        state.paint.setColor4f(SkColor4f{0, 0, 0, 0.6});
        state.paint.setPathEffect(SkDashPathEffect::Make(state.dash, 2, state.dashStep + state.dashOffset));
        canvas.drawPath(path, state.paint);
    }

    void onDrawBackground(SkCanvas& canvas) override {
        canvas.drawImage(state.backgroundImage, 0, 0, SkSamplingOptions(), &state.backgroundPaint);
        canvas.drawImage(state.image, 0, 0);
        canvas.drawColor(SkColor4f{.0, .0, .0, .6});
    }

    void onDraw(SkCanvas& canvas) override {
        if (!HasContent(state.selectedRect))
            return;
        canvas.clipRect(SkRect::Make(state.selectedRect));
        canvas.drawImage(state.image, 0, 0);
    }

    void onDrawForeground(SkCanvas& canvas) override {
        auto rect = HasContent(state.selectedRect) ? state.dstRect() : state.selectWindow(state.selectDepth).rect;
        drawRect(canvas, rect);
        state.dashOffset += 0.5;
        state.redraw();

        drawMagnifier(canvas, state.magnifierSize);
    }

    USE_RET SkRect getMagnifierRect(const f32 size) const {
        const SkPoint pos(
            std::clamp<f32>(static_cast<f32>(state.mousePos.fX), 0, static_cast<f32>(state.image->width())),
            std::clamp<f32>(static_cast<f32>(state.mousePos.fY), 0, static_cast<f32>(state.image->height())));
        return SkRect::MakeXYWH(pos.fX - size / 2, pos.fY - size / 2, size, size);
    }

    void drawMagnifier(SkCanvas& canvas, const f32 size) const {
        const f32 crossWidth = size / static_cast<f32>(state.magnifierRange);

        // 抓取当前界面，以便显示放大的图像
        const auto img = canvas.getSurface()->makeImageSnapshot();
        static auto value = 0.0f;
        static auto shadow = SkImageFilters::Blur(10, 10, nullptr);
        const auto mX = static_cast<f32>(state.mousePos.fX);
        const auto mY = static_cast<f32>(state.mousePos.fY);
        auto src = getMagnifierRect(static_cast<f32>(state.magnifierRange));
        src.offset(0.5, 0.5);

        SkIRect mouseRect;
        for (auto rect : state.screenRects) {
            if (rect.contains(state.mousePos.fX, state.mousePos.fY)) {
                mouseRect = rect;
                break;
            }
        }

        const f32 d = -state.magnifierSize * 0.06f;

        const auto dts = SkRect::MakeXYWH(
            mX + (mX < static_cast<f32>(mouseRect.fRight) - size - d ? d : -size - d),
            mY + (mY < static_cast<f32>(mouseRect.fBottom) - size - d ? d : -size - d),
            size,
            size);
        const auto path = SkPath::Oval(dts);

        canvas.save();

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setStyle(SkPaint::kStrokeAndFill_Style);
        paint.setImageFilter(shadow);
        paint.setStrokeWidth(2);
        paint.setColor4f(SkColor4f{0.5, 0.5, 0.5, 0.6});
        canvas.drawPath(path, paint);

        canvas.restore();

        canvas.clipPath(path);
        canvas.drawImageRect(img, src, dts, SkSamplingOptions(), nullptr, SkCanvas::kStrict_SrcRectConstraint);

        paint.reset();
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(crossWidth);
        const auto v = value > 1 ? 2 - value : value;
        paint.setColor4f(SkColor4f{v, v, v, 0.6});
        canvas.drawLine(dts.fLeft + size / 2, dts.fTop, dts.fLeft + size / 2, dts.fBottom, paint);
        canvas.drawLine(dts.fLeft, dts.fTop + size / 2, dts.fRight, dts.fTop + size / 2, paint);

        value += 0.01;
        if (value > 2)
            value -= 2;
        state.redraw();
    }

    bool onhMouseWheel(SkIPoint pos, const int delta, const int modifier) override {
        if (modifier & MK_CONTROL) {
            state.magnifierRange += delta > 0 ? -10 : 10;
            state.magnifierRange = std::clamp(state.magnifierRange, 10, 100);
        } else {
            state.magnifierSize += delta > 0 ? 25 : -25;
            state.magnifierSize = std::clamp<f32>(state.magnifierSize, 100, 500);
        }
        state.redraw();
        return true;
    }

    void updateCursor(const SkIPoint& pos) const {
        const auto rect = &state.selectedRect;
        switch (state.actionType) {
        case CaptureState::ACTION_TYPE_NONE:
        case CaptureState::ACTION_TYPE_WAIT_RECT:
            if (HasContent(*rect)) {
                if (const auto cursor = getNest(*rect, pos); cursor != CaptureState::CURSOR_NULL) {
                    state.cursorType = cursor;
                } else {
                    if (rect->contains(pos.fX, pos.fY)) {
                        state.cursorType = CaptureState::CURSOR_MOVE;
                    } else if (state.actionType == CaptureState::ACTION_TYPE_WAIT_RECT) {
                        state.cursorType = CaptureState::CURSOR_SELECT;
                    } else {
                        state.cursorType = CaptureState::CURSOR_NONE;
                    }
                }
            }
            break;
        case CaptureState::ACTION_TYPE_RECT:
            rect->fRight = pos.fX;
            rect->fBottom = pos.fY;
            break;
        case CaptureState::ACTION_TYPE_MODIFY_RECT_1:
            rect->fLeft = pos.fX;
            rect->fTop = pos.fY;
            break;
        case CaptureState::ACTION_TYPE_MODIFY_RECT_2:
            rect->fTop = pos.fY;
            break;
        case CaptureState::ACTION_TYPE_MODIFY_RECT_3:
            rect->fRight = pos.fX;
            rect->fTop = pos.fY;
            break;
        case CaptureState::ACTION_TYPE_MODIFY_RECT_4:
            rect->fLeft = pos.fX;
            break;
        case CaptureState::ACTION_TYPE_MODIFY_RECT_5:
            rect->offset(state.mousePos - state.lastMousePos);
            break;
        case CaptureState::ACTION_TYPE_MODIFY_RECT_6:
            rect->fRight = pos.fX;
            break;
        case CaptureState::ACTION_TYPE_MODIFY_RECT_7:
            rect->fLeft = pos.fX;
            rect->fBottom = pos.fY;
            break;
        case CaptureState::ACTION_TYPE_MODIFY_RECT_8:
            rect->fBottom = pos.fY;
            break;
        case CaptureState::ACTION_TYPE_MODIFY_RECT_9:
            rect->fRight = pos.fX;
            rect->fBottom = pos.fY;
            break;
        default:
            break;
        }

        state.setCursor();
    }

    bool onMouseMove(const SkIPoint pos) override {
        state.mousePos.set(pos.fX, pos.fY);
        updateCursor(pos);
        state.lastMousePos.set(pos.fX, pos.fY);

        state.redraw();
        return true;
    }

    bool onMouseDown(const SkIPoint pos, const MouseKey key) override {
        switch (key) {
        case MOUSE_LEFT: {
            if (HasContent(state.selectedRect)) {
                switch (state.cursorType) {
                case CaptureState::CURSOR_NONE:
                    state.selectedRect = SkIRect::MakeXYWH(pos.fX, pos.fY, 0, 0);
                    state.actionType = CaptureState::ACTION_TYPE_RECT;
                    state.cursorType = CaptureState::CURSOR_SELECT;
                    break;
                case CaptureState::CURSOR_SELECT:
                    state.actionType = CaptureState::ACTION_TYPE_RECT;
                    break;
                case CaptureState::CURSOR_MOVE:
                    state.actionType = CaptureState::ACTION_TYPE_MODIFY_RECT_5;
                    state.cursorType = CaptureState::CURSOR_MOVE;
                    break;
                case CaptureState::CURSOR_DIAGONAL:
                    state.actionType = inTl(state.selectedRect, pos, POINT_D) ? CaptureState::ACTION_TYPE_MODIFY_RECT_1
                                                                              : CaptureState::ACTION_TYPE_MODIFY_RECT_9;
                    break;
                case CaptureState::CURSOR_DIAGONAL_BACK:
                    state.actionType = inTr(state.selectedRect, pos, POINT_D) ? CaptureState::ACTION_TYPE_MODIFY_RECT_3
                                                                              : CaptureState::ACTION_TYPE_MODIFY_RECT_7;
                    break;
                case CaptureState::CURSOR_HORIZONTAL:
                    state.actionType = inLeft(state.selectedRect, pos, POINT_D)
                        ? CaptureState::ACTION_TYPE_MODIFY_RECT_4
                        : CaptureState::ACTION_TYPE_MODIFY_RECT_6;
                    break;
                case CaptureState::CURSOR_VERTICAL:
                    state.actionType = inTop(state.selectedRect, pos, POINT_D)
                        ? CaptureState::ACTION_TYPE_MODIFY_RECT_2
                        : CaptureState::ACTION_TYPE_MODIFY_RECT_8;
                    break;
                default:
                    return false;
                }
            } else {
                state.selectedRect = SkIRect::MakeXYWH(pos.fX, pos.fY, 0, 0);
                state.actionType = CaptureState::ACTION_TYPE_RECT;
                state.cursorType = CaptureState::CURSOR_SELECT;
            }
            state.redraw();
            break;
        }
        case MOUSE_RIGHT:
            if (state.actionType == CaptureState::ACTION_TYPE_RECT) {
                state.actionType = CaptureState::ACTION_TYPE_WAIT_RECT;
                state.reset();
                state.redraw();
            }
        default:
            return false;
        }
        return true;
    }

    bool onMouseUp(const SkIPoint pos, const MouseKey key) override {
        switch (key) {
        case MOUSE_LEFT:
            if (HasContent(state.selectedRect)) {
                state.cursorType = CaptureState::CURSOR_NONE;
                state.selectedRect.sort();
            } else {
                state.selectedRect = state.selectWindow(state.selectDepth).rect;
            }

            state.actionType = CaptureState::ACTION_TYPE_NONE;
            updateCursor(pos);
            state.redraw();
            break;
        case MOUSE_RIGHT:
            if (state.actionType == CaptureState::ACTION_TYPE_RECT) {
                state.actionType = CaptureState::ACTION_TYPE_NONE;
                state.reset();
            }
        default:
            return false;
        }
        return true;
    }

    bool onKeyDown(const u32 vk) override {
        switch (vk) {
        case VK_ESCAPE:
            if (HasContent(state.selectedRect)) {
                state.reset();
                state.redraw();
            } else {
                state.hide();
            }
            break;
        case VK_RETURN:
            commit();
            break;
        case VK_LEFT: {
            POINT p;
            GetCursorPos(&p);
            SetCursorPos(p.x - 1, p.y);
            break;
        }
        case VK_UP: {
            POINT p;
            GetCursorPos(&p);
            SetCursorPos(p.x, p.y - 1);
            break;
        }
        case VK_RIGHT: {
            POINT p;
            GetCursorPos(&p);
            SetCursorPos(p.x + 1, p.y);
            break;
        }
        case VK_DOWN: {
            POINT p;
            GetCursorPos(&p);
            SetCursorPos(p.x, p.y + 1);
            break;
        }
        case VK_OEM_PLUS:
            if (state.selectDepth < 5)
                state.selectDepth++;
            break;
        case VK_OEM_MINUS:
            if (state.selectDepth > 0)
                state.selectDepth--;
            break;
        default:
            break;
        }
        return true;
    }

    bool onKeyUp(u32 vk) override { return true; }

    USE_RET sk_sp<SkImage> getDstImage() const {
        if (state.selectedRect.width() == 0 || state.selectedRect.height() == 0)
            return state.image;

        const auto dstRect = state.dstRect();
        const auto surface = state.window()->surface()->makeSurface(dstRect.width(), dstRect.height());
        const auto canvas = surface->getCanvas();

        canvas->drawImageRect(
            state.image,
            SkRect::Make(dstRect),
            SkRect::MakeIWH(dstRect.width(), dstRect.height()),
            SkSamplingOptions(),
            nullptr,
            SkCanvas::kStrict_SrcRectConstraint);

        const auto img = surface->makeImageSnapshot();
        return img->makeRasterImage(state.window()->context().get(), SkImage::kDisallow_CachingHint);
    }

    void commit() {
        struct Bm : BITMAPV5HEADER
        {
            BYTE* data;
        };

        const auto img = getDstImage();

        const auto w = img->imageInfo().width();
        const auto h = img->imageInfo().height();
        const auto minRowBytes = img->imageInfo().minRowBytes();
        const auto handle = GlobalAlloc(GHND, sizeof(BITMAPV5HEADER) + minRowBytes * h);
        handle || ShowErrorExit();

        if (img->peekPixels(&_pixmap)) {
            const auto bm = static_cast<Bm*>(GlobalLock(handle));
            bm->bV5Size = sizeof(BITMAPV5HEADER);
            bm->bV5Width = w;
            bm->bV5Height = -h;
            bm->bV5Planes = 1;
            bm->bV5BitCount = 32;
            bm->bV5Compression = BI_BITFIELDS;
            bm->bV5SizeImage = minRowBytes * h;
            bm->bV5AlphaMask = 0xff000000;
            bm->bV5RedMask = 0x000000ff;
            bm->bV5GreenMask = 0x0000ff00;
            bm->bV5BlueMask = 0x00ff0000;
            bm->bV5CSType = LCS_sRGB;
            bm->bV5Intent = LCS_GM_IMAGES;

            const auto addr = _pixmap.addr();

            memcpy(&bm->data, addr, minRowBytes * h);

            OpenClipboard(state.window()->hWnd()) || ShowErrorExit();

            guard _clipboard([] { CloseClipboard(); });
            EmptyClipboard() || ShowErrorExit();
            GlobalUnlock(handle) || ShowErrorExit();
            SetClipboardData(CF_DIBV5, handle) || ShowErrorExit();
        }

        state.hide();
    }

    static constexpr u32 POINT_D = 5;

    template <typename N>
    static bool inRange(const N v, const N min, const N max) {
        return v >= min && v <= max;
    }

    template <typename N>
    static bool inDistance(const N x1, const N y1, const N x2, const N y2, const N d) {
        return SkPoint::Length(static_cast<f32>(x1 - x2), static_cast<f32>(y1 - y2)) < static_cast<f32>(d);
    }

    static bool inTl(const SkIRect& rect, const SkIPoint& pos, const i32 d) {
        return inDistance(rect.fLeft, rect.fTop, pos.fX, pos.fY, d);
    }

    static bool inTr(const SkIRect& rect, const SkIPoint& pos, const i32 d) {
        return inDistance(rect.fRight, rect.fTop, pos.fX, pos.fY, d);
    }

    static bool inBr(const SkIRect& rect, const SkIPoint& pos, const i32 d) {
        return inDistance(rect.fRight, rect.fBottom, pos.fX, pos.fY, d);
    }

    static bool inBl(const SkIRect& rect, const SkIPoint& pos, const i32 d) {
        return inDistance(rect.fLeft, rect.fBottom, pos.fX, pos.fY, d);
    }

    static bool inLeft(const SkIRect& rect, const SkIPoint& pos, const i32 d) {
        return std::abs(pos.fX - rect.fLeft) < d;
    }

    static bool inTop(const SkIRect& rect, const SkIPoint& pos, const i32 d) {
        return std::abs(pos.fY - rect.fTop) < d;
    }

    static CaptureState::CursorType getNest(const SkIRect& rect, const SkIPoint& pos) {
        if (inTl(rect, pos, POINT_D) || inBr(rect, pos, POINT_D)) {
            return CaptureState::CursorType::CURSOR_DIAGONAL;
        }
        if (inBl(rect, pos, POINT_D) || inTr(rect, pos, POINT_D)) {
            return CaptureState::CURSOR_DIAGONAL_BACK;
        }

        if (inRange(pos.fY, rect.fTop, rect.fBottom)) {
            if (std::abs(pos.fX - rect.fLeft) < POINT_D || std::abs(pos.fX - rect.fRight) < POINT_D) {
                return CaptureState::CURSOR_HORIZONTAL;
            }
        } else if (inRange(pos.fX, rect.fLeft, rect.fRight)) {
            if (std::abs(pos.fY - rect.fTop) < POINT_D || std::abs(pos.fY - rect.fBottom) < POINT_D) {
                return CaptureState::CURSOR_VERTICAL;
            }
        }

        return CaptureState::CURSOR_NULL;
    }
};

void CaptureState::enableLowMode() {
    PROCESS_POWER_THROTTLING_STATE pps{
        .Version = THREAD_POWER_THROTTLING_CURRENT_VERSION,
        .ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED,
        .StateMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED,
    };
    SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &pps, sizeof(THREAD_POWER_THROTTLING_STATE)) ||
        ShowErrorExit();

    MEMORY_PRIORITY_INFORMATION pms{MEMORY_PRIORITY_VERY_LOW};
    SetProcessInformation(GetCurrentProcess(), ProcessMemoryPriority, &pms, sizeof(MEMORY_PRIORITY_INFORMATION)) ||
        ShowErrorExit();

    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS) || ShowErrorExit();
}

void CaptureState::disableLowMode() {
    PROCESS_POWER_THROTTLING_STATE pps{
        .Version = THREAD_POWER_THROTTLING_CURRENT_VERSION,
        .ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED,
        .StateMask = 0,
    };
    SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &pps, sizeof(THREAD_POWER_THROTTLING_STATE)) ||
        ShowErrorExit();

    MEMORY_PRIORITY_INFORMATION pms{MEMORY_PRIORITY_NORMAL};
    SetProcessInformation(GetCurrentProcess(), ProcessMemoryPriority, &pms, sizeof(MEMORY_PRIORITY_INFORMATION)) ||
        ShowErrorExit();

    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS) || ShowErrorExit();
}

CaptureState::CaptureState() :
    actionType(ACTION_TYPE_WAIT_RECT), cursorType(CURSOR_SELECT), mousePos(), lastMousePos() {
    dash[0] = dashStep;
    dash[1] = dashStep;
}

CaptureState::~CaptureState() = default;

void CaptureState::onCreate(Window* window) {
    _window = window;
    NOTIFYICONDATA notifyIconData;
    notifyIconData.cbSize = sizeof(notifyIconData);
    notifyIconData.hWnd = _window->hWnd();
    notifyIconData.uID = 0;
    notifyIconData.hIcon = LoadIcon(GetModuleHandle(nullptr), TEXT("APP_ICON"));
    lstrcpy(notifyIconData.szTip, TEXT("h-shot"));
    notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    notifyIconData.uCallbackMessage = WM_USER;

    Shell_NotifyIcon(NIM_ADD, &notifyIconData) || ShowErrorExit(TEXT("Could not create tray icon"));

    _tray_menu = CreatePopupMenu();
    AppendMenu(_tray_menu, MF_STRING, TMI_EXIT, TEXT("Exit"));
}

void CaptureState::onCreated() {
    capture();
    SetWindowDisplayAffinity(_window->hWnd(), WDA_EXCLUDEFROMCAPTURE);
    show();
}

Window* CaptureState::window() const { return _window; }

void CaptureState::capture() {
    screenRects = Window::getDesktopRects();
    desktopRect = Window::getDesktopRect(screenRects);
    windowInfos = Window::getWindowRects();
    auto desktop = GetDesktopWindow();
    HDC desktopDc = GetWindowDC(desktop);
    desktopDc || ShowErrorExit();

    guard _desktopDc([desktopDc, desktop] { ReleaseDC(desktop, desktopDc); });

    HDC mdc = CreateCompatibleDC(desktopDc);
    mdc || ShowErrorExit();
    guard _mdc([mdc] { DeleteDC(mdc); });

    auto hbitmap = CreateCompatibleBitmap(desktopDc, desktopRect.width(), desktopRect.height());
    hbitmap || ShowErrorExit();
    guard _hbitmap([hbitmap] { DeleteObject(hbitmap); });

    SelectObject(mdc, hbitmap);

    BitBlt(
        mdc, 0, 0, desktopRect.width(), desktopRect.height(), desktopDc, desktopRect.x(), desktopRect.y(), SRCCOPY) ||
        ShowErrorExit();

    //
    //

    const auto imageInfo = SkImageInfo::MakeN32Premul(desktopRect.width(), desktopRect.height());

    DWORD bufferSize = imageInfo.minRowBytes() * imageInfo.height();

    BITMAPINFO bitmapInfo = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = desktopRect.width(),
            .biHeight = -desktopRect.height(),
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB,
        }};

    auto data = SkData::MakeUninitialized(bufferSize);

    GetDIBits(mdc, hbitmap, 0, desktopRect.height(), data->writable_data(), &bitmapInfo, DIB_RGB_COLORS) ||
        ShowErrorExit();

    auto image = SkImages::RasterFromData(imageInfo, data, imageInfo.minRowBytes());

    auto surface = _window->surface()->makeSurface(image->width(), image->height());
    auto canvas = surface->getCanvas();

    canvas->drawColor(SkColors::kTransparent);

    SkPath path;
    for (auto& r : screenRects) {
        r.offset(-desktopRect.fLeft, -desktopRect.fTop);
        path.addRect(SkRect::Make(r));
    }
    canvas->clipPath(path);
    canvas->drawImage(image, 0, 0);

    this->image = surface->makeImageSnapshot();

    auto size = std::max(std::min(image->imageInfo().width(), image->imageInfo().height()) / 100, 10);
    surface = _window->surface()->makeSurface(size * 2, size * 2);
    canvas = surface->getCanvas();

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kFill_Style);
    auto step = static_cast<f32>(size);
    for (int x = 0; x < 2; x += 1) {
        for (int y = 0; y < 2; y += 1) {
            auto v = (x + y & 1) & 1 ? 0.8f : 0.3f;
            paint.setColor4f(SkColor4f{v, v, v, 0.6});
            canvas->drawRect(
                SkRect::MakeXYWH(static_cast<f32>(x) * step, static_cast<f32>(y) * step, step, step), paint);
        }
    }

    backgroundImage = surface->makeImageSnapshot();

    backgroundPaint.setImageFilter(
        SkImageFilters::Tile(
            SkRect::MakeWH(static_cast<f32>(backgroundImage->width()), static_cast<f32>(backgroundImage->height())),
            SkRect::MakeWH(static_cast<f32>(image->width()), static_cast<f32>(image->height())),
            nullptr));
}

void CaptureState::setCursor() const {
    auto c = cursorType;
    if (HasContent(selectedRect)) {
        if (c == CURSOR_DIAGONAL || c == CURSOR_DIAGONAL_BACK) {
            int rc = 0;
            if (selectedRect.fRight < selectedRect.fLeft)
                rc++;
            if (selectedRect.fBottom < selectedRect.fTop)
                rc++;
            if (rc & 1) {
                c = c == CURSOR_DIAGONAL ? CURSOR_DIAGONAL_BACK : CURSOR_DIAGONAL;
            }
        }
    }
    switch (c) {
    case CURSOR_NONE:
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
        break;
    case CURSOR_MOVE:
        SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
        break;
    case CURSOR_SELECT:
        SetCursor(LoadCursor(nullptr, IDC_CROSS));
        break;
    case CURSOR_DIAGONAL:
        SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
        break;
    case CURSOR_DIAGONAL_BACK:
        SetCursor(LoadCursor(nullptr, IDC_SIZENESW));
        break;
    case CURSOR_HORIZONTAL:
        SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
        break;
    case CURSOR_VERTICAL:
        SetCursor(LoadCursor(nullptr, IDC_SIZENS));
        break;
    default:;
    }
}

View* CaptureState::build() { return new CaptureView(this); }

SkIRect CaptureState::dstRect() const {
    auto rect = SkIRect::MakeWH(image->width(), image->height());
    rect.intersect(selectedRect.makeSorted());
    return rect;
}

void CaptureState::redraw() const {
    if (_window)
        _window->redraw();
}

void CaptureState::reset() {
    actionType = ACTION_TYPE_WAIT_RECT;
    cursorType = CURSOR_SELECT;
    selectedRect.setLTRB(0, 0, 0, 0);
    dashOffset = 0;
}

void CaptureState::show() {
    const auto rect = Window::getDesktopRect();
    _window->setRect(rect.fLeft - 1, rect.fTop, 1, 1);
    _window->hide();
    reset();
    capture();
    _window->show();
    _window->setRect(rect, HWND_TOPMOST);
    SetForegroundWindow(_window->hWnd());
}

void CaptureState::hide() {
    _window->setRect(0, 0, 1, 1, nullptr);
    _window->hide();
    image = nullptr;
    reset();
    enableLowMode();
}

void CaptureState::onHotKey(const HotKey key) {
    disableLowMode();
    show();
}

void CaptureState::onTray(const TryMenuItemId id) {
    switch (id) {
    case TMI_OPEN:
        onHotKey(HOTKEY_SCREEN);
        break;
    case TMI_EXIT:
        _window->destroy();
        break;
    }
}

void CaptureState::onCustomMessage(const WPARAM wParam, const LPARAM lParam) {
    POINT pt;
    GetCursorPos(&pt);
    switch (lParam) {
    case WM_HOTKEY:
        onHotKey(static_cast<HotKey>(wParam));
        break;
    case WM_LBUTTONDOWN:
        onTray(TMI_OPEN);
        break;
    case WM_RBUTTONDOWN:
        onTray(
            static_cast<TryMenuItemId>(
                TrackPopupMenuEx(_tray_menu, TPM_RETURNCMD, pt.x, pt.y, _window->hWnd(), nullptr)));
        break;
    default:
        break;
    }
}

void CaptureState::onFocus(const bool focus) {
    if (_window->isVisible() && !focus) {
        _window->setOrder(HWND_TOPMOST);
        SetForegroundWindow(_window->hWnd());
    }
}

WinInfo CaptureState::selectWindow(const int maxDepth, const bool translate) const {
    POINT pos;
    GetCursorPos(&pos);

    WinInfo r = {nullptr, SkIRect(desktopRect)};
    auto infos = &windowInfos;
    int depth = 0;

loop:
    for (const WinInfo& info : *infos) {
        if (info.rect.contains(pos.x, pos.y)) {
            depth++;
            r = info;
            infos = &info.children;
            if (depth < maxDepth)
                goto loop;
            break;
        }
    }

    if (translate) {
        r.rect.offset(-desktopRect.fLeft, -desktopRect.fTop);
    }
    return r;
}
