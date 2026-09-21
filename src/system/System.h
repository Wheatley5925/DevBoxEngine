#pragma once

namespace dbx::System {

// Initialize system state
void begin();

// Per-frame maintenance
void update();

// Request a return to OS at a safe point
void requestReturnToOS();

// True if a return was requested
bool returnToOSRequested();

// Perform the target-specific return behavior through DevBoxSDK.
void returnToOS();

}
