#include "Assets.h"
#include <cstring>
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
    if (!path || !path[0]) return out;

    FILE* f = fopen(path, "rb");
    if (!f) return out;

    struct SprHeader {
        char magic[4];
        uint16_t width;
        uint16_t height;
    } hdr{};

    if (fread(&hdr, 1, sizeof(hdr), f) != sizeof(hdr)) {
        fclose(f);
        return out;
    }

    if (memcmp(hdr.magic, "SPR4", 4) != 0 || hdr.width == 0 || hdr.height == 0) {
        fclose(f);
        return out;
    }

    const size_t dataSize = ((hdr.width + 1) >> 1) * hdr.height;
    uint8_t* pixels = new (std::nothrow) uint8_t[dataSize];
    if (!pixels) {
        fclose(f);
        return out;
    }

    if (fread(pixels, 1, dataSize, f) != dataSize) {
        delete[] pixels;
        fclose(f);
        return out;
    }

    fclose(f);

    out.width = hdr.width;
    out.height = hdr.height;
    out.data = pixels;
    out.ownsMemory = true;
    return out;
}

AtlasAsset loadAtlas(const char* path) {
    AtlasAsset out;
    if (!path || !path[0]) return out;

    FILE* f = fopen(path, "rb");
    if (!f) return out;

    struct AtlHeader {
        char magic[4];
        uint16_t width;
        uint16_t height;
        uint16_t frameCount;
        uint8_t transparentColor;
        uint8_t reserved;
    } hdr{};

    if (fread(&hdr, 1, sizeof(hdr), f) != sizeof(hdr)) {
        fclose(f);
        return out;
    }

    if (memcmp(hdr.magic, "ATL4", 4) != 0 ||
        hdr.width == 0 || hdr.height == 0 || hdr.frameCount == 0) {
        fclose(f);
        return out;
    }

    struct AtlasFrameDisk {
        char name[16];
        uint16_t x;
        uint16_t y;
        uint16_t w;
        uint16_t h;
        int16_t originX;
        int16_t originY;
    };

    dbx::SpriteFrame* frames = new (std::nothrow) dbx::SpriteFrame[hdr.frameCount];
    if (!frames) {
        fclose(f);
        return out;
    }

    char (*names)[16] = new (std::nothrow) char[hdr.frameCount][16];
    if (!names) {
        delete[] frames;
        fclose(f);
        return out;
    }

    for (uint16_t i = 0; i < hdr.frameCount; ++i) {
        AtlasFrameDisk fr{};
        if (fread(&fr, 1, sizeof(fr), f) != sizeof(fr)) {
            delete[] names;
            delete[] frames;
            fclose(f);
            return out;
        }

        memcpy(names[i], fr.name, 16);
        names[i][15] = '\0';

        frames[i].x = fr.x;
        frames[i].y = fr.y;
        frames[i].w = fr.w;
        frames[i].h = fr.h;
        frames[i].originX = fr.originX;
        frames[i].originY = fr.originY;
    }

    const size_t dataSize = ((hdr.width + 1) >> 1) * hdr.height;
    uint8_t* pixels = new (std::nothrow) uint8_t[dataSize];
    if (!pixels) {
        delete[] names;
        delete[] frames;
        fclose(f);
        return out;
    }

    if (fread(pixels, 1, dataSize, f) != dataSize) {
        delete[] pixels;
        delete[] names;
        delete[] frames;
        fclose(f);
        return out;
    }

    fclose(f);

    out.bitmap.width = hdr.width;
    out.bitmap.height = hdr.height;
    out.bitmap.data = pixels;
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
