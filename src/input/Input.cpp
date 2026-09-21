#include "Input.h"

#include <stdint.h>
#include <DevBoxSDK.h>

namespace dbx::Input {
namespace {

constexpr int kButtonCount = 10;

// Engine logical buttons -> DevBoxSDK raw indices.
// Adjust only if SDK order differs from this logical order.
constexpr int8_t kButtonMap[kButtonCount] = {
    0, // Up
    1, // Down
    2, // Left
    3, // Right
    4, // A
    5, // B
    6, // X
    7, // Y
    8, // Select
    9  // Start
};

bool s_curr[kButtonCount] = {};
bool s_prev[kButtonCount] = {};

inline int toIndex(Button b) {
    return static_cast<int>(b);
}

inline bool readRawButton(int logicalIndex) {
    const int rawIndex = kButtonMap[logicalIndex];
    if (rawIndex < 0) {
        return false;
    }
    return !buttonRaw(rawIndex);
}

} // namespace

void begin() {
    for (int i = 0; i < kButtonCount; ++i) {
        s_curr[i] = false;
        s_prev[i] = false;
    }
}

void update() {
    for (int i = 0; i < kButtonCount; ++i) {
        s_prev[i] = s_curr[i];
        s_curr[i] = readRawButton(i);
    }
}

bool held(Button b) {
    const int i = toIndex(b);
    return (i >= 0 && i < kButtonCount) ? s_curr[i] : false;
}

bool pressed(Button b) {
    const int i = toIndex(b);
    return (i >= 0 && i < kButtonCount) ? (s_curr[i] && !s_prev[i]) : false;
}

bool released(Button b) {
    const int i = toIndex(b);
    return (i >= 0 && i < kButtonCount) ? (!s_curr[i] && s_prev[i]) : false;
}

} // namespace dbx::Input
