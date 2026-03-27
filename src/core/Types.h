#pragma once

#include <stdint.h>

namespace dbx {

constexpr int ScreenWidth  = 256;
constexpr int ScreenHeight = 128;

// Temporary logical button layout for the engine layer.
// Adjust when we wire the real input mapping.
enum class Button : uint8_t {
    Up = 0,
    Down,
    Left,
    Right,
    A,
    B,
    X,
    Y,
    Start,
    Select
};

struct Vec2i {
    int16_t x = 0;
    int16_t y = 0;
};

struct Vec2f {
    float x = 0.0f;
    float y = 0.0f;
};

struct Rect {
    int16_t x = 0;
    int16_t y = 0;
    int16_t w = 0;
    int16_t h = 0;
};

} // namespace dbx
