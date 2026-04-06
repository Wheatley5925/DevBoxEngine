#include <DevBoxEngine.h>
#include "MainScene.h"

dbx::Engine engine;
MainScene mainScene;

static bool wasReturnComboHeld = false;

static bool shouldReturnToOS() {
    return dbx::Input::held(dbx::Button::Start) &&
           dbx::Input::held(dbx::Button::Select);
}

void setup() {
    engine.begin(&mainScene);
}

void loop() {
    engine.tick();

    const bool comboHeld = shouldReturnToOS();
    if (comboHeld && !wasReturnComboHeld) {
        dbx::System::requestReturnToOS();
    }
    wasReturnComboHeld = comboHeld;
}
