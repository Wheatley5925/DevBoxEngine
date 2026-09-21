#include "System.h"

#include <DevBoxSDK.h>

namespace dbx::System {

static bool s_returnRequested = false;

void begin() {
    s_returnRequested = false;
}

void update() {
    // V1: nothing else needed
}

void requestReturnToOS() {
    s_returnRequested = true;
}

bool returnToOSRequested() {
    return s_returnRequested;
}

void returnToOS() {
    s_returnRequested = false;
    devboxReturnToOS();
}

}
