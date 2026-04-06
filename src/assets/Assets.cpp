#include "Assets.h"

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <stdlib.h>
#include <new>

namespace dbx::Assets {
namespace {

#pragma pack(push, 1)

struct SprHeader {
    char magic[4];      // "SPR4"
    uint16_t width;
    uint16_t height;
};

struct AtlHeader {
    char magic[4];      // "ATL4"
    uint16_t width;
    uint16_t height;
    uint16_t frameCount;
    uint8_t transparentColor;
    uint8_t reserved;
};

struct AtlasFrameDisk {
    char name[16];
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    int16_t originX;
    int16_t originY;
};

#pragma pack(pop)

bool isValidSprHeader(const SprHeader& h) {
    return h.magic[0] == 'S' &&
           h.magic[1] == 'P' &&
           h.magic[2] == 'R' &&
           h.magic[3] == '4' &&
           h.width > 0 &&
           h.height > 0;
}

bool isValidAtlHeader(const AtlHeader& h) {
    return h.magic[0] == 'A' &&
           h.magic[1] == 'T' &&
           h.magic[2] == 'L' &&
           h.magic[3] == '4' &&
           h.width > 0 &&
           h.height > 0 &&
           h.frameCount > 0;
}

int calcBitmapBytes(int width, int height) {
    return ((width + 1) >> 1) * height;
}

} // namespace

bool begin() {
    return true;
}

void update() {
    // placeholder for future async loading / cache work
}

BitmapAsset loadBitmap(const char* path) {
    BitmapAsset out;

    if (!path || !path[0]) {
        return out;
    }

    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
        return out;
    }

    SprHeader hdr{};
    const int hdrRead = f.read(reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr));
    if (hdrRead != (int)sizeof(hdr) || !isValidSprHeader(hdr)) {
        f.close();
        return out;
    }

    const int bytes = calcBitmapBytes(hdr.width, hdr.height);
    if (bytes <= 0) {
        f.close();
        return out;
    }

    uint8_t* mem = static_cast<uint8_t*>(malloc(bytes));
    if (!mem) {
        f.close();
        return out;
    }

    const int rd = f.read(mem, bytes);
    f.close();

    if (rd != bytes) {
        free(mem);
        return out;
    }

    out.data = mem;
    out.width = static_cast<int16_t>(hdr.width);
    out.height = static_cast<int16_t>(hdr.height);
    out.ownsMemory = true;
    return out;
}

AtlasAsset loadAtlas(const char* path) {
    AtlasAsset out;

    if (!path || !path[0]) {
        return out;
    }

    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
        return out;
    }

    AtlHeader hdr{};
    const int hdrRead = f.read(reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr));
    if (hdrRead != (int)sizeof(hdr) || !isValidAtlHeader(hdr)) {
        f.close();
        return out;
    }

    SpriteFrame* frames = new (std::nothrow) SpriteFrame[hdr.frameCount];
    if (!frames) {
        f.close();
        return out;
    }

    for (uint16_t i = 0; i < hdr.frameCount; ++i) {
        AtlasFrameDisk fr{};
        const int rd = f.read(reinterpret_cast<uint8_t*>(&fr), sizeof(fr));
        if (rd != (int)sizeof(fr)) {
            delete[] frames;
	    delete[] names;
            f.close();
            return out;
        }

	std::memcpy(names[i], fr.name, 16);
	names[i][15] = '\0';

        frames[i].x = fr.x;
        frames[i].y = fr.y;
        frames[i].w = fr.w;
        frames[i].h = fr.h;
        frames[i].originX = fr.originX;
        frames[i].originY = fr.originY;
    }

    const int bytes = calcBitmapBytes(hdr.width, hdr.height);
    if (bytes <= 0) {
        delete[] frames;
        f.close();
        return out;
    }

    uint8_t* pixels = static_cast<uint8_t*>(malloc(bytes));
    if (!pixels) {
        delete[] frames;
        f.close();
        return out;
    }

    const int rd = f.read(pixels, bytes);
    f.close();

    if (rd != bytes) {
        free(pixels);
        delete[] frames;
        return out;
    }

    out.bitmap.data = pixels;
    out.bitmap.width = static_cast<int16_t>(hdr.width);
    out.bitmap.height = static_cast<int16_t>(hdr.height);
    out.bitmap.ownsMemory = true;

    out.frames = frames;
    out.frameNames = names;
    out.frameCount = hdr.frameCount;
    out.transparentColor = hdr.transparentColor;
    out.ownsFrames = true;

    return out;
}

void unload(BitmapAsset& asset) {
    if (asset.ownsMemory && asset.data) {
        free(asset.data);
    }

    asset.data = nullptr;
    asset.width = 0;
    asset.height = 0;
    asset.ownsMemory = false;
}

void unload(AtlasAsset& atlas) {
    unload(atlas.bitmap);

    if (atlas.ownsFrames && atlas.frames) {
        delete[] atlas.frames;
    }

    atlas.frames = nullptr;
    atlas.frameNames = nullptr;
    atlas.frameCount = 0;
    atlas.transparentColor = 0;
    atlas.ownsFrames = false;
}

} // namespace dbx::Assets
