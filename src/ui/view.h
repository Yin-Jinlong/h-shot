#pragma once

#include <memory>
#include <vector>

#include "../capture_state.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkSize.h"

class State;

enum MouseKey
{
    MOUSE_LEFT,
    MOUSE_MIDDLE,
    MOUSE_RIGHT,
};

class View
{
protected:
    State* _state;
    std::vector<View*> _children;

public:
    explicit View(State* state) : _state(state) {};
    virtual ~View();

    void calcSize(SkSize maxSize);
    virtual SkSize onCalcSize(SkSize maxSize);

    void draw(SkCanvas& canvas);
    virtual void onDrawBackground(SkCanvas& canvas);
    virtual void onDraw(SkCanvas& canvas);
    virtual void onDrawForeground(SkCanvas& canvas);

    bool dispatchMouseWheel(SkIPoint pos, i16 delta, int modifier);
    bool dispatchMouseMove(SkIPoint pos);
    void dispatchMouseLeave(SkIPoint pos);

    virtual bool onhMouseWheel(SkIPoint pos, int delta, int modifier);
    virtual bool onMouseMove(SkIPoint pos);
    virtual void onMouseLeave(SkIPoint pos);

    bool dispatchMouseDown(SkIPoint pos, MouseKey key);
    bool dispatchMouseUp(SkIPoint pos, MouseKey key);

    virtual bool onMouseDown(SkIPoint pos, MouseKey key);
    virtual bool onMouseUp(SkIPoint pos, MouseKey key);

    bool dispatchKeyDown(u32 vk);
    bool dispatchKeyUp(u32 vk);

    virtual bool onKeyDown(u32 vk);
    virtual bool onKeyUp(u32 vk);
};
