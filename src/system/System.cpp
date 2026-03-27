#include "System.h"

#include <Arduino.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"

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

[[noreturn]] void returnToOS() {
    const esp_partition_t* next = esp_ota_get_next_update_partition(nullptr);
    if (!next) {
        Serial.println("returnToOS: no next OTA partition");
        delay(100);
        esp_restart();
    }

    esp_err_t err = esp_ota_set_boot_partition(next);
    if (err != ESP_OK) {
        Serial.printf("returnToOS: esp_ota_set_boot_partition failed: %d\n", (int)err);
        delay(100);
        esp_restart();
    }

    delay(50);
    esp_restart();

    while (true) {
        // should never get here
    }
}

}
