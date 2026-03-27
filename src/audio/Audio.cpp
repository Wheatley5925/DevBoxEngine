#include "Audio.h"

#include <Arduino.h>
#include <DevBoxSDK.h>

#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <stdio.h>
#include <string.h>

namespace dbx::Audio {
namespace {

enum class CommandType : uint8_t {
    Play,
    Stop
};

enum class PlaybackKind : uint8_t {
    Sfx,
    Music
};

struct AudioCommand {
    CommandType type = CommandType::Stop;
    PlaybackKind kind = PlaybackKind::Sfx;
    char path[160] = {};
};

// One-slot queue: latest command wins.
// This is intentional for V1. If play is requested repeatedly,
// only the newest request matters.
constexpr UBaseType_t kQueueLen = 1;

// Small chunks improve stop/interrupt responsiveness.
constexpr size_t kChunkSize = 1024;

// Tune later if needed.
constexpr uint32_t kTaskStackBytes = 4096;
constexpr UBaseType_t kTaskPriority = 3;
constexpr BaseType_t kTaskCore = 0;

QueueHandle_t s_queue = nullptr;
TaskHandle_t s_task = nullptr;

volatile bool s_initialized = false;
volatile bool s_playing = false;

bool makePlayCommand(AudioCommand& cmd, const char* path, PlaybackKind kind) {
    if (!path || !path[0]) {
        return false;
    }

    cmd.type = CommandType::Play;
    cmd.kind = kind;

    const size_t n = strlen(path);
    if (n >= sizeof(cmd.path)) {
        return false;
    }

    memcpy(cmd.path, path, n + 1);
    return true;
}

bool enqueueLatest(const AudioCommand& cmd) {
    if (!s_initialized || !s_queue) {
        return false;
    }

    // Queue length is 1, so overwrite is the intended behavior.
    xQueueOverwrite(s_queue, &cmd);
    return true;
}

bool pollCommand(AudioCommand& cmd) {
    if (!s_queue) return false;
    return xQueueReceive(s_queue, &cmd, 0) == pdTRUE;
}

void stopPlaybackState() {
    s_playing = false;
}

bool streamWavFileUntilInterrupted(const AudioCommand& initialCmd, AudioCommand& nextCmd, bool& hasNext) {
    hasNext = false;

    FILE* f = fopen(initialCmd.path, "rb");
    if (!f) {
        stopPlaybackState();
        return false;
    }

    // Mirrors the current DevBoxSDK helper behavior:
    // skip the fixed 44-byte WAV header and stream payload.
    fseek(f, 44, SEEK_SET);

    uint8_t buf[kChunkSize];
    s_playing = true;

    for (;;) {
        // Check pending commands before each file read
        AudioCommand cmd;
        while (pollCommand(cmd)) {
            if (cmd.type == CommandType::Stop) {
                fclose(f);
                stopPlaybackState();
                return false;
            }
            if (cmd.type == CommandType::Play) {
                fclose(f);
                nextCmd = cmd;
                hasNext = true;
                stopPlaybackState();
                return true;
            }
        }

        const size_t bytesRead = fread(buf, 1, sizeof(buf), f);
        if (bytesRead == 0) {
            fclose(f);
            stopPlaybackState();
            return false;
        }

        size_t off = 0;
        while (off < bytesRead) {
            // Check commands between write chunks too,
            // so stop/replace is not delayed too much.
            AudioCommand cmd;
            while (pollCommand(cmd)) {
                if (cmd.type == CommandType::Stop) {
                    fclose(f);
                    stopPlaybackState();
                    return false;
                }
                if (cmd.type == CommandType::Play) {
                    fclose(f);
                    nextCmd = cmd;
                    hasNext = true;
                    stopPlaybackState();
                    return true;
                }
            }

            size_t written = 0;
            const esp_err_t err =
                i2s_write(I2S_NUM_0, buf + off, bytesRead - off, &written, portMAX_DELAY);

            if (err != ESP_OK || written == 0) {
                fclose(f);
                stopPlaybackState();
                return false;
            }

            off += written;
        }
    }
}

void audioTask(void* /*arg*/) {
    AudioCommand cmd;

    for (;;) {
        if (xQueueReceive(s_queue, &cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (cmd.type == CommandType::Stop) {
            stopPlaybackState();
            continue;
        }

        if (cmd.type == CommandType::Play) {
            AudioCommand current = cmd;

            for (;;) {
                AudioCommand nextCmd;
                bool hasNext = false;

                const bool switched =
                    streamWavFileUntilInterrupted(current, nextCmd, hasNext);

                if (switched && hasNext) {
                    current = nextCmd;
                    continue;
                }

                break;
            }
        }
    }
}

} // namespace

bool begin() {
    if (s_initialized) {
        return true;
    }

    s_queue = xQueueCreate(kQueueLen, sizeof(AudioCommand));
    if (!s_queue) {
        return false;
    }

    const BaseType_t ok = xTaskCreatePinnedToCore(
        audioTask,
        "dbxAudio",
        kTaskStackBytes,
        nullptr,
        kTaskPriority,
        &s_task,
        kTaskCore
    );

    if (ok != pdPASS) {
        vQueueDelete(s_queue);
        s_queue = nullptr;
        return false;
    }

    s_initialized = true;
    s_playing = false;
    return true;
}

void update() {
    // V1 task-based audio does not need a frame update.
}

bool playSfx(const char* path) {
    AudioCommand cmd;
    if (!makePlayCommand(cmd, path, PlaybackKind::Sfx)) {
        return false;
    }
    return enqueueLatest(cmd);
}

bool playMusic(const char* path) {
    AudioCommand cmd;
    if (!makePlayCommand(cmd, path, PlaybackKind::Music)) {
        return false;
    }
    return enqueueLatest(cmd);
}

void stop() {
    AudioCommand cmd;
    cmd.type = CommandType::Stop;
    cmd.kind = PlaybackKind::Music;
    cmd.path[0] = '\0';
    enqueueLatest(cmd);
}

bool isPlaying() {
    return s_playing;
}

} // namespace dbx::Audio
