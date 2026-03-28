#pragma once

#include <stdint.h>
#include "BitmapAsset.h"
#include "../gfx/Sprite.h"

namespace dbx {

// Owned atlas asset.
// Owns both bitmap memory and frame metadata, and can expose a non-owning
// SpriteAtlas view for the renderer.
struct AtlasAsset {
    BitmapAsset bitmap;
    SpriteFrame* frames = nullptr;
    uint16_t frameCount = 0;
    uint8_t transparentColor = 0;
    bool ownsFrames = false;

    bool valid() const {
        return bitmap.valid() && frames != nullptr && frameCount > 0;
    }

    SpriteAtlas view() const {
        SpriteAtlas out;
        out.bitmap = bitmap.view();
        out.frames = frames;
        out.frameCount = frameCount;
        return out;
    }
};

} // namespace dbx
