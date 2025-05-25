#pragma once

#include <functional>
#include <utility>

struct guard
{
    explicit guard(std::function<void(void)> fn) : _fn(std::move(fn)) {}

    ~guard() {
        if (_fn)
            _fn();
    }

private:
    std::function<void(void)> _fn;
};
