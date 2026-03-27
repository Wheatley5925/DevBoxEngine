#pragma once

#include <stdint.h>
#include "Bitmap.h"

namespace dbx {

// One sprite frame inside an atlas texture.
struct SpriteFrame {
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t w = 0;
    uint16_t h = 0;

    // Origin / pivot relative to the destination point.
    // Example: if originX = w/2 and originY = h/2,
    // draw position becomes the sprite center.
    int16_t originX = 0;
    int16_t originY = 0;
};

// Non-owning atlas view.
// The atlas bitmap and frame table are owned elsewhere.
struct SpriteAtlas {
    BitmapView bitmap;
    const SpriteFrame* frames = nullptr;
    uint16_t frameCount = 0;

    bool valid() const {
        return bitmap.valid() && frames != nullptr && frameCount > 0;
    }

    const SpriteFrame* getFrame(uint16_t id) const {
        if (!valid() || id >= frameCount) return nullptr;
        return &frames[id];
    }
};

} // namespace dbx
