#pragma once

#include "BitmapAsset.h"
#include "AtlasAsset.h"

namespace dbx::Assets {

// Initializes engine-side asset state.
bool begin();

// Per-frame asset service update.
// Placeholder for future async loading / cache work.
void update();

// SPR4 file format:
//   4 bytes magic: 'SPR4'
//   uint16 width
//   uint16 height
//   packed 4bpp payload
BitmapAsset loadBitmap(const char* path);

// ATL4 file format:
//   4 bytes magic: 'ATL4'
//   uint16 width
//   uint16 height
//   uint16 frameCount
//   uint8 transparentColor
//   uint8 reserved
//   frameCount frame records
//   packed 4bpp atlas payload
AtlasAsset loadAtlas(const char* path);

void unload(BitmapAsset& asset);
void unload(AtlasAsset& atlas);

} // namespace dbx::Assets
