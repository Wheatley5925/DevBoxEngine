#include "Renderer.h"

#include <DevBoxSDK.h>
#include <algorithm>

namespace dbx {
namespace {

// ------------------------------------------------------------
// Internal 4bpp helpers
// ------------------------------------------------------------

inline uint8_t clamp4(int v) {
    return (v < 0) ? 0 : (v > 15 ? 15 : (uint8_t)v);
}

inline int strideBytes4(int width) {
    return (width + 1) >> 1;
}

inline uint8_t getPixel4(const uint8_t* data, int width, int x, int y) {
    const int stride = strideBytes4(width);
    const uint8_t b = data[y * stride + (x >> 1)];
    return (x & 1) ? (b >> 4) : (b & 0x0F);
}

inline void setFbPixel4(int x, int y, uint8_t gray) {
    if ((unsigned)x >= 256u || (unsigned)y >= 128u) {
        return;
    }

    gray &= 0x0F;
    const int idx = y * 128 + (x >> 1); // 256 px / 2 = 128 bytes per row
    uint8_t b = fb4[idx];

    if (x & 1) {
        // odd x -> high nibble
        b = (b & 0x0F) | (gray << 4);
    } else {
        // even x -> low nibble
        b = (b & 0xF0) | gray;
    }

    fb4[idx] = b;
}

} // namespace

void Renderer::clear(uint8_t gray) {
    clearGray(clamp4(gray));
}

void Renderer::present() {
    sendToDisplay();
}

void Renderer::setCamera(int x, int y) {
    m_cameraX = x;
    m_cameraY = y;
}

void Renderer::moveCamera(int dx, int dy) {
    m_cameraX += dx;
    m_cameraY += dy;
}

void Renderer::resetCamera() {
    m_cameraX = 0;
    m_cameraY = 0;
}

void Renderer::fillRect(int x, int y, int w, int h, uint8_t gray) {
    x -= m_cameraX;
    y -= m_cameraY;

    if (w <= 0 || h <= 0) return;
    drawBox(x, y, w, h, clamp4(gray));
}

void Renderer::drawText(int x, int y, const char* text, uint8_t gray) {
    if (!text) return;

    x -= m_cameraX;
    y -= m_cameraY;
    ::drawText(x, y, text, clamp4(gray), false, 0);
}

void Renderer::drawBitmap(
    const BitmapView& bmp,
    int x,
    int y,
    bool transparent,
    uint8_t transparentColor
) {
    if (!bmp.valid()) return;

    drawBitmapRect(
        bmp,
        x,
        y,
        0,
        0,
        bmp.width,
        bmp.height,
        transparent,
        transparentColor
    );
}

void Renderer::drawBitmapRect(
    const BitmapView& bmp,
    int dstX,
    int dstY,
    int srcX,
    int srcY,
    int srcW,
    int srcH,
    bool transparent,
    uint8_t transparentColor
) {
    if (!bmp.valid()) return;
    if (srcW <= 0 || srcH <= 0) return;

    // Apply camera
    dstX -= m_cameraX;
    dstY -= m_cameraY;

    // Clip source rectangle against the bitmap itself
    if (srcX < 0) {
        srcW += srcX;
        dstX -= srcX;
        srcX = 0;
    }
    if (srcY < 0) {
        srcH += srcY;
        dstY -= srcY;
        srcY = 0;
    }
    if (srcX + srcW > bmp.width) {
        srcW = bmp.width - srcX;
    }
    if (srcY + srcH > bmp.height) {
        srcH = bmp.height - srcY;
    }

    if (srcW <= 0 || srcH <= 0) return;

    // Clip destination rectangle against the screen
    if (dstX < 0) {
        srcX += -dstX;
        srcW -= -dstX;
        dstX = 0;
    }
    if (dstY < 0) {
        srcY += -dstY;
        srcH -= -dstY;
        dstY = 0;
    }
    if (dstX + srcW > 256) {
        srcW = 256 - dstX;
    }
    if (dstY + srcH > 128) {
        srcH = 128 - dstY;
    }

    if (srcW <= 0 || srcH <= 0) return;

    // Fast path: full opaque bitmap at exact size.
    // Use SDK primitive directly when possible.
    if (!transparent &&
        srcX == 0 && srcY == 0 &&
        srcW == bmp.width && srcH == bmp.height) {
        drawGrayBitmap(dstX, dstY, bmp.data, bmp.width, bmp.height);
        return;
    }

    // Generic engine-side blitter.
    const uint8_t key = transparentColor & 0x0F;

    for (int row = 0; row < srcH; ++row) {
        for (int col = 0; col < srcW; ++col) {
            const uint8_t px = getPixel4(bmp.data, bmp.width, srcX + col, srcY + row);
            if (transparent && px == key) {
                continue;
            }
            setFbPixel4(dstX + col, dstY + row, px);
        }
    }
}

void Renderer::drawSprite(
    const SpriteAtlas& atlas,
    uint16_t frameId,
    int x,
    int y,
    bool transparent,
    uint8_t transparentColor
) {
    const SpriteFrame* fr = atlas.getFrame(frameId);
    if (!fr) return;

    drawBitmapRect(
        atlas.bitmap,
        x - fr->originX,
        y - fr->originY,
        fr->x,
        fr->y,
        fr->w,
        fr->h,
        transparent,
        transparentColor
    );
}

} // namespace dbx
