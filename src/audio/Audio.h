#pragma once

#include <stdint.h>

namespace dbx::Audio {

enum class Status : uint8_t {
    None,
    FileOpenFailed,
    InvalidWav,
    CacheFull,
    OutOfMemory,
    ReadFailed,
    SfxCached,
    SfxPlaying
};

// Starts the engine-side audio worker.
// Hardware init is still done by Engine::begin() through DevBoxSDK.
bool begin();

// Kept for API stability. The worker does not need per-frame work.
void update();

// Request playback of a short sound.
// Non-blocking from the caller's point of view.
// Up to four sound effects can play over the music at once.
bool playSfx(const char* path);

// Queue an SFX file for loading into RAM before its first playback.
bool preloadSfx(const char* path);

// Request playback of music.
// Non-blocking from the caller's point of view.
// Replaces the current music without stopping active sound effects.
bool playMusic(const char* path);

// Stop music and all active sound effects.
void stop();

// Returns whether the worker currently considers a file active.
bool isPlaying();

// Last audio-worker diagnostic, safe to read from the game thread.
Status status();
const char* statusText();

} // namespace dbx::Audio
