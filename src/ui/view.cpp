//
// Created by yjl on 2025/5/27.
//

#include "view.h"

#include "state.h"

View::~View() {
    for (const auto child : _children) {
        delete child;
    }
    _children.clear();
}

void View::calcSize(const SkSize maxSize) { // NOLINT(*-no-recursion)
    for (const auto child : _children) {
        child->calcSize(maxSize);
    }
    _state->attr.size = onCalcSize(maxSize);
}

SkSize View::onCalcSize(const SkSize maxSize) { return maxSize; }

void View::draw(SkCanvas& canvas) { // NOLINT(*-no-recursion)
    const auto id = canvas.save();
    onDrawBackground(canvas);
    canvas.restoreToCount(id);
    canvas.save();
    onDraw(canvas);
    canvas.restoreToCount(id);
    canvas.save();
    onDrawForeground(canvas);
    canvas.restoreToCount(id);
}

void View::onDrawBackground(SkCanvas& canvas) {}

void View::onDraw(SkCanvas& canvas) { // NOLINT(*-no-recursion)
    const auto id = canvas.save();
    for (const auto child : _children) {
        canvas.restoreToCount(id);
        canvas.save();
        canvas.translate(_state->attr.pos.fX, _state->attr.pos.fY);
        canvas.clipRect(SkRect::MakeSize(_state->attr.size));
        child->draw(canvas);
    }
}

void View::onDrawForeground(SkCanvas& canvas) {}

bool PosInChild(const SkIPoint& pos, const ViewAttr& child) {
    return SkRect::MakeXYWH(child.pos.fX, child.pos.fY, child.size.width(), child.size.height())
        .contains(static_cast<f32>(pos.fX), static_cast<f32>(pos.fY));
}

bool View::dispatchMouseWheel(const SkIPoint pos, const i16 delta, const int modifier) { // NOLINT(*-no-recursion)
    for (const auto child : _children) {
        if (const auto attr = child->_state->attr; attr.mouseOn) {
            if (child->dispatchMouseWheel(pos, delta, modifier)) {
                return true;
            }
        }
    }
    return onhMouseWheel(pos, delta, modifier);
}

bool View::dispatchMouseMove(const SkIPoint pos) { // NOLINT(*-no-recursion)
    for (const auto child : _children) {
        if (auto attr = child->_state->attr; PosInChild(pos, attr)) { // 鼠标在子view上
            attr.mouseOn = true;
            if (child->dispatchMouseMove(pos)) {
                return true;
            }
        } else if (attr.mouseOn) {
            child->dispatchMouseLeave(pos);
        }
    }
    return onMouseMove(pos);
}

void View::dispatchMouseLeave(const SkIPoint pos) {
    for (const auto child : _children) {
        const auto attr = child->_state->attr;
        if (attr.mouseOn) {
            child->onMouseLeave(pos);
        }
    }
    onMouseLeave(pos);
}

bool View::onhMouseWheel(SkIPoint pos, int delta, int modifier) { return false; }

bool View::onMouseMove(SkIPoint pos) { return false; }

void View::onMouseLeave(SkIPoint pos) {}

bool View::dispatchMouseDown(const SkIPoint pos, const MouseKey key) { // NOLINT(*-no-recursion)
    for (const auto child : _children) {
        if (auto attr = child->_state->attr; PosInChild(pos, attr)) {
            if (child->dispatchMouseDown(pos, key)) {
                return true;
            }
        }
    }
    return onMouseDown(pos, key);
}

bool View::dispatchMouseUp(const SkIPoint pos, const MouseKey key) { // NOLINT(*-no-recursion)
    for (const auto child : _children) {
        if (auto attr = child->_state->attr; PosInChild(pos, attr)) {
            if (child->dispatchMouseUp(pos, key)) {
                return true;
            }
        }
    }
    return onMouseUp(pos, key);
}

bool View::onMouseDown(SkIPoint pos, MouseKey key) { return false; }

bool View::onMouseUp(SkIPoint pos, MouseKey key) { return false; }

bool View::dispatchKeyDown(const u32 vk) { // NOLINT(*-no-recursion)
    for (const auto child : _children) {
        if (const auto attr = child->_state->attr; attr.mouseOn) {
            if (child->dispatchKeyDown(vk)) {
                return true;
            }
        }
    }
    return onKeyDown(vk);
}

bool View::dispatchKeyUp(const u32 vk) { // NOLINT(*-no-recursion)
    for (const auto child : _children) {
        if (const auto attr = child->_state->attr; attr.mouseOn) {
            if (child->dispatchKeyUp(vk)) {
                return true;
            }
        }
    }
    return onKeyUp(vk);
}

bool View::onKeyDown(u32 vk) { return false; }

bool View::onKeyUp(u32 vk) { return false; }
