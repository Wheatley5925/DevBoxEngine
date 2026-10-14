#include <DevBoxTarget.h>

#if DEVBOX_TARGET_IS_DESKTOP

#include <DevBoxEngine.h>
#include <DevBoxSDK.h>
#include "src/MainScene.h"

namespace {
dbx::Engine engine;
MainScene mainScene;
bool wasReturnComboHeld = false;

bool shouldReturnToOS() {
    return dbx::Input::held(dbx::Button::Start) &&
           dbx::Input::held(dbx::Button::Select);
}
}

int main() {
    if (!engine.begin(&mainScene)) return 1;

    while (!devboxApplicationShouldClose()) {
        engine.tick();

        const bool comboHeld = shouldReturnToOS();
        if (comboHeld && !wasReturnComboHeld) {
            dbx::System::requestReturnToOS();
        }
        wasReturnComboHeld = comboHeld;

        static uint32_t lastSample = devboxMicros();
        static unsigned frames = 0;

        ++frames;
        uint32_t now = devboxMicros();
        uint32_t elapsed = now - lastSample;
        if (elapsed >= 1000000) {
            std::printf("FPS: %.1f\n", frames * 1000000.0 / elapsed);
            frames = 0;
            lastSample = now;
        }
    }

    return 0;
}

#endif // DEVBOX_TARGET_IS_DESKTOP
