#include <DevBoxTarget.h>

#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX

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
    }

    return 0;
}

#endif // DEVBOX_TARGET_LINUX
