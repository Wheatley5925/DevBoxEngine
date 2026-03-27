#pragma once

#include <stdint.h>
#include "../core/Types.h"
#include "Bitmap.h"
#include "Sprite.h"

namespace dbx {

// Renderer is the game-facing drawing API.
// It wraps DevBoxSDK display primitives and adds an engine-side 4bpp blitter
// so game code can draw sub-rectangles from sprite atlases.
//
// Design notes:
// - V1 has no scaling/rotation.
// - V1 uses a simple integer camera.
// - V1 supports optional color-key transparency for 4bpp sprite drawing.
class Renderer {
public:
    Renderer() = default;

    // Frame control
    void clear(uint8_t gray = 0);
    void present();

    // Camera
    void setCamera(int x, int y);
    void moveCamera(int dx, int dy);
    void resetCamera();

    int cameraX() const { return m_cameraX; }
    int cameraY() const { return m_cameraY; }

    // Basic primitives
    void fillRect(int x, int y, int w, int h, uint8_t gray = 15);
    void drawText(int x, int y, const char* text, uint8_t gray = 15);

    // World-space bitmap drawing
    void drawBitmap(
        const BitmapView& bmp,
        int x,
        int y,
        bool transparent = false,
        uint8_t transparentColor = 0
    );

    void drawBitmapRect(
        const BitmapView& bmp,
        int dstX,
        int dstY,
        int srcX,
        int srcY,
        int srcW,
        int srcH,
        bool transparent = false,
        uint8_t transparentColor = 0
    );

    // Sprite atlas helpers
    void drawSprite(
        const SpriteAtlas& atlas,
        uint16_t frameId,
        int x,
        int y,
        bool transparent = true,
        uint8_t transparentColor = 0
    );

private:
    int m_cameraX = 0;
    int m_cameraY = 0;
};

} // namespace dbx
