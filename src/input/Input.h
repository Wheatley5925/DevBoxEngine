#pragma once

#include "../core/Types.h"

namespace dbx::Input {

// Initializes internal button state buffers.
// Does not initialize hardware; Engine::begin() already does that.
void begin();

// Polls raw button state from DevBoxSDK and updates
// current/previous frame snapshots.
// Call once per frame from Engine::tick().
void update();

// True while the button is currently held down.
bool held(Button b);

// True only on the frame the button changed from up -> down.
bool pressed(Button b);

// True only on the frame the button changed from down -> up.
bool released(Button b);

} // namespace dbx::Input
