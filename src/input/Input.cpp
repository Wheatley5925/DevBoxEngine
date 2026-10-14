#include "Input.h"

#include <stdint.h>
#include <DevBoxSDK.h>

#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX
#include <cstdlib>
#endif

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

} // namespace

void begin() {
    for (int i = 0; i < kButtonCount; ++i) {
        s_curr[i] = false;
        s_prev[i] = false;
    }
}

void update() {
#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX
    // Keep the previous polling path available for a controlled performance comparison.
    static const bool legacyPolling = std::getenv("DEVBOX_INPUT_LEGACY") != nullptr;
    if (legacyPolling) {
        for (int i = 0; i < kButtonCount; ++i) {
            s_prev[i] = s_curr[i];
            const int rawIndex = kButtonMap[i];
            s_curr[i] = rawIndex >= 0 && !buttonRaw(rawIndex);
        }
        return;
    }
#endif
    bool released[kButtonCount] = {};
    readButtonsRaw(released, kButtonCount);
    for (int i = 0; i < kButtonCount; ++i) {
        s_prev[i] = s_curr[i];
        const int rawIndex = kButtonMap[i];
        s_curr[i] = rawIndex >= 0 && rawIndex < kButtonCount && !released[rawIndex];
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
