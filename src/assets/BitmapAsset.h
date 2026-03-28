#pragma once

#include <stdint.h>
#include "../gfx/Bitmap.h"

namespace dbx {

// Owned bitmap asset stored in RAM.
// It can be converted into a non-owning BitmapView for rendering.
struct BitmapAsset {
    uint8_t* data = nullptr;
    int16_t width = 0;
    int16_t height = 0;
    bool ownsMemory = false;

    bool valid() const {
        return data != nullptr && width > 0 && height > 0;
    }

    int strideBytes() const {
        return (width + 1) >> 1;
    }

    int byteSize() const {
        return strideBytes() * height;
    }

    BitmapView view() const {
        BitmapView v;
        v.data = data;
        v.width = width;
        v.height = height;
        return v;
    }
};

} // namespace dbx
