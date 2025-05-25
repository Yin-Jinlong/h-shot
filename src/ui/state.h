#pragma once

#include "include/core/SkPoint.h"
#include "include/core/SkSize.h"

#include <windows.h>

class Window;
class View;

struct ViewAttr
{
    SkPoint pos = {};
    SkSize size = {};
    bool mouseOn = false;
};


class State
{
public:
    virtual ~State() = default;
    ViewAttr attr;

    virtual View* build() = 0;
};

class RootState : public State
{
public:
    virtual void onCreate(Window* window) = 0;
    virtual void onCreated() = 0;
    virtual void onCustomMessage(WPARAM wParam, LPARAM lParam) = 0;
    virtual void onFocus(bool focus) = 0;
};
