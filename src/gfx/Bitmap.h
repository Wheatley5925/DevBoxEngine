#pragma once

#include <stdint.h>

namespace dbx {

// BitmapView is a non-owning view of a 4bpp packed grayscale bitmap.
// Pixel format matches DevBoxSDK display.h:
//
// - 4 bits per pixel
// - row-major
// - each row uses ceil(width / 2) bytes
// - even x = low nibble, odd x = high nibble
//
// This type belongs to gfx because the renderer understands the layout.
// Asset loading/ownership can be added later in assets/.
struct BitmapView {
    const uint8_t* data = nullptr;
    int16_t width = 0;
    int16_t height = 0;

    bool valid() const {
        return data != nullptr && width > 0 && height > 0;
    }

    int strideBytes() const {
        return (width + 1) >> 1;
    }
};

} // namespace dbx
