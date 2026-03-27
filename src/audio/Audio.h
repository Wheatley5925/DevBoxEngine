#pragma once

namespace dbx::Audio {

// Starts the engine-side audio worker.
// Hardware init is still done by Engine::begin() through DevBoxSDK.
bool begin();

// Kept for API stability. V1 task-based audio does not need per-frame work yet.
void update();

// Request playback of a short sound.
// Non-blocking from the caller's point of view.
// In V1, this uses the same single playback channel as music.
bool playSfx(const char* path);

// Request playback of music.
// Non-blocking from the caller's point of view.
// In V1, this interrupts any currently playing file and starts the new one.
bool playMusic(const char* path);

// Request stop of current playback.
void stop();

// Returns whether the worker currently considers a file active.
bool isPlaying();

} // namespace dbx::Audio
