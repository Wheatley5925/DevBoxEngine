#include "Audio.h"

#include <DevBoxSDK.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>

#if DEVBOX_TARGET == DEVBOX_TARGET_ESP32
#include <esp_heap_caps.h>
#endif

namespace dbx::Audio {
namespace {

enum class CommandType : uint8_t { Play, Preload, Stop };
enum class PlaybackKind : uint8_t { Sfx, Music };

struct AudioCommand {
    CommandType type = CommandType::Stop;
    PlaybackKind kind = PlaybackKind::Sfx;
    char path[160] = {};
};

struct SfxClip {
    char path[160] = {};
    int16_t* samples = nullptr;
    size_t sampleCount = 0;
};

struct SfxVoice {
    const SfxClip* clip = nullptr;
    size_t position = 0;
};

constexpr size_t kQueueLength = 8;
constexpr size_t kChunkBytes = 1024;
constexpr size_t kChunkSamples = kChunkBytes / sizeof(int16_t);
constexpr size_t kSfxVoiceCount = 4;
constexpr size_t kSfxCacheSize = 8;

constexpr uint32_t kTaskStackBytes = 12288;
constexpr unsigned kTaskPriority = 3;
constexpr int kTaskCore = 0;

DevBoxQueue* s_queue = nullptr;
std::atomic<bool> s_initialized{false};
std::atomic<bool> s_playing{false};
std::atomic<Status> s_status{Status::None};

int16_t* allocateSfxSamples(size_t sampleCount) {
#if DEVBOX_TARGET == DEVBOX_TARGET_ESP32
    return static_cast<int16_t*>(heap_caps_malloc(
        sampleCount * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
    return new (std::nothrow) int16_t[sampleCount];
#endif
}

void freeSfxSamples(int16_t* samples) {
#if DEVBOX_TARGET == DEVBOX_TARGET_ESP32
    heap_caps_free(samples);
#else
    delete[] samples;
#endif
}

bool makePlayCommand(AudioCommand& command, const char* path,
                     PlaybackKind kind) {
    if (!path || !path[0]) return false;
    const size_t length = std::strlen(path);
    if (length >= sizeof(command.path)) return false;

    command.type = CommandType::Play;
    command.kind = kind;
    std::memcpy(command.path, path, length + 1);
    return true;
}

bool enqueue(const AudioCommand& command) {
    return s_initialized.load() && s_queue &&
           devboxQueueSend(s_queue, &command);
}

struct WavFile {
    FILE* file = nullptr;
    size_t samplesRemaining = 0;
};

uint16_t readLe16(const uint8_t* bytes) {
    return static_cast<uint16_t>(bytes[0]) |
           (static_cast<uint16_t>(bytes[1]) << 8);
}

uint32_t readLe32(const uint8_t* bytes) {
    return static_cast<uint32_t>(bytes[0]) |
           (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) |
           (static_cast<uint32_t>(bytes[3]) << 24);
}

WavFile openWav(const char* path) {
    const std::string resolvedPath = sdPath(path);
    if (resolvedPath.empty()) {
        s_status.store(Status::FileOpenFailed);
        devboxPrintf("Audio path could not be resolved: %s\n", path ? path : "(null)");
        return {};
    }

    FILE* file = std::fopen(resolvedPath.c_str(), "rb");
    if (!file) {
        s_status.store(Status::FileOpenFailed);
        devboxPrintf("Audio file could not be opened: %s\n", resolvedPath.c_str());
        return {};
    }
    const auto invalid = [&]() -> WavFile {
        s_status.store(Status::InvalidWav);
        devboxPrintf("Invalid or unsupported WAV: %s\n", resolvedPath.c_str());
        std::fclose(file);
        return {};
    };

    // Check the container's bounds before seeking over any chunks.
    if (std::fseek(file, 0, SEEK_END) != 0) return invalid();
    const long fileSize = std::ftell(file);
    if (fileSize < 12 || std::fseek(file, 0, SEEK_SET) != 0) return invalid();
    uint8_t header[12];
    if (std::fread(header, 1, sizeof(header), file) != sizeof(header) ||
        std::memcmp(header, "RIFF", 4) != 0 ||
        std::memcmp(header + 8, "WAVE", 4) != 0) return invalid();
    const uint32_t riffBytes = readLe32(header + 4);
    if (riffBytes < 4 || static_cast<uint64_t>(riffBytes) + 8 >
                         static_cast<uint64_t>(fileSize)) return invalid();
    uint32_t remaining = riffBytes - 4;
    bool formatFound = false;

    while (remaining >= 8) {
        uint8_t chunk[8];
        if (std::fread(chunk, 1, sizeof(chunk), file) != sizeof(chunk)) return invalid();
        remaining -= 8;
        const uint32_t size = readLe32(chunk + 4);
        // RIFF chunks with an odd payload length have one padding byte.
        const uint64_t paddedSize = static_cast<uint64_t>(size) + (size & 1u);
        if (paddedSize > remaining) return invalid();

        if (std::memcmp(chunk, "data", 4) == 0) {
            if (!formatFound || size % 4 != 0) return invalid();
            return {file, size / sizeof(int16_t)};
        }

        uint32_t consumed = 0;
        if (std::memcmp(chunk, "fmt ", 4) == 0) {
            uint8_t format[16];
            if (size < sizeof(format) ||
                std::fread(format, 1, sizeof(format), file) != sizeof(format)) return invalid();
            // The mixer and SDK expect 44.1 kHz stereo, signed 16-bit PCM.
            if (readLe16(format) != 1 || readLe16(format + 2) != 2 ||
                readLe32(format + 4) != 44100 || readLe32(format + 8) != 176400 ||
                readLe16(format + 12) != 4 || readLe16(format + 14) != 16) return invalid();
            formatFound = true;
            consumed = sizeof(format);
        }
        if (std::fseek(file, static_cast<long>(paddedSize - consumed), SEEK_CUR) != 0) return invalid();
        remaining -= static_cast<uint32_t>(paddedSize);
    }
    return invalid();
}

void closeFile(WavFile& wav) {
    if (wav.file) std::fclose(wav.file);
    wav = {};
}

const SfxClip* findOrLoadSfx(const char* path,
                             std::array<SfxClip, kSfxCacheSize>& cache) {
    for (const SfxClip& clip : cache) {
        if (clip.samples && std::strcmp(clip.path, path) == 0) return &clip;
    }

    SfxClip* slot = nullptr;
    for (SfxClip& clip : cache) {
        if (!clip.samples) {
            slot = &clip;
            break;
        }
    }
    if (!slot) {
        s_status.store(Status::CacheFull);
        return nullptr;
    }

    WavFile wav = openWav(path);
    if (!wav.file) return nullptr;

    const size_t sampleCount = wav.samplesRemaining;
    if (sampleCount == 0) {
        closeFile(wav);
        return nullptr;
    }
    int16_t* samples = allocateSfxSamples(sampleCount);
    if (!samples) {
        s_status.store(Status::OutOfMemory);
        devboxPrintf("Not enough memory to cache SFX (%u bytes): %s\n",
                     static_cast<unsigned>(sampleCount * sizeof(int16_t)), path);
        closeFile(wav);
        return nullptr;
    }

    const size_t loaded =
        std::fread(samples, sizeof(int16_t), sampleCount, wav.file);
    closeFile(wav);
    if (loaded != sampleCount) {
        s_status.store(Status::ReadFailed);
        freeSfxSamples(samples);
        return nullptr;
    }

    std::strncpy(slot->path, path, sizeof(slot->path) - 1);
    slot->path[sizeof(slot->path) - 1] = '\0';
    slot->samples = samples;
    slot->sampleCount = sampleCount;
    s_status.store(Status::SfxCached);
    devboxPrintf("Cached SFX (%u bytes): %s\n",
                 static_cast<unsigned>(sampleCount * sizeof(int16_t)), path);
    return slot;
}

bool hasActiveSource(const WavFile& music,
                     const std::array<SfxVoice, kSfxVoiceCount>& voices) {
    if (music.file) return true;
    for (const SfxVoice& voice : voices) {
        if (voice.clip) return true;
    }
    return false;
}

void stopAll(WavFile& music, std::array<SfxVoice, kSfxVoiceCount>& voices) {
    closeFile(music);
    for (SfxVoice& voice : voices) voice = {};
}

size_t mixMusic(WavFile& music, int32_t* mix) {
    if (!music.file) return 0;

    int16_t input[kChunkSamples];
    const size_t requested = music.samplesRemaining < kChunkSamples
                                 ? music.samplesRemaining : kChunkSamples;
    const size_t count = std::fread(input, sizeof(int16_t), requested, music.file);
    music.samplesRemaining -= count;
    if (count < requested || music.samplesRemaining == 0) closeFile(music);
    if (count == 0) {
        closeFile(music);
        return 0;
    }
    for (size_t i = 0; i < count; ++i) mix[i] += input[i];
    return count;
}

size_t mixVoice(SfxVoice& voice, int32_t* mix) {
    if (!voice.clip) return 0;

    const size_t remaining = voice.clip->sampleCount - voice.position;
    const size_t count = remaining < kChunkSamples ? remaining : kChunkSamples;
    for (size_t i = 0; i < count; ++i) {
        mix[i] += voice.clip->samples[voice.position + i];
    }
    voice.position += count;
    if (voice.position >= voice.clip->sampleCount) voice = {};
    return count;
}

int16_t clampSample(int32_t sample) {
    if (sample > INT16_MAX) return INT16_MAX;
    if (sample < INT16_MIN) return INT16_MIN;
    return static_cast<int16_t>(sample);
}

void applyCommand(const AudioCommand& command,
                  WavFile& music,
                  std::array<SfxClip, kSfxCacheSize>& cache,
                  std::array<SfxVoice, kSfxVoiceCount>& voices,
                  size_t& nextVoice) {
    if (command.type == CommandType::Stop) {
        stopAll(music, voices);
        return;
    }

    if (command.type == CommandType::Preload) {
        findOrLoadSfx(command.path, cache);
        return;
    }

    if (command.kind == PlaybackKind::Music) {
        closeFile(music);
        music = openWav(command.path);
        return;
    }

    const SfxClip* clip = findOrLoadSfx(command.path, cache);
    if (!clip) return;

    size_t slot = kSfxVoiceCount;
    for (size_t i = 0; i < kSfxVoiceCount; ++i) {
        if (!voices[i].clip) {
            slot = i;
            break;
        }
    }
    if (slot == kSfxVoiceCount) slot = nextVoice;
    voices[slot] = {clip, 0};
    nextVoice = (slot + 1) % kSfxVoiceCount;
    s_status.store(Status::SfxPlaying);
    devboxPrintf("Playing SFX on voice %u: %s\n",
                 static_cast<unsigned>(slot), command.path);
}

bool writeMixedChunk(int16_t* output, size_t sampleCount) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(output);
    const size_t byteCount = sampleCount * sizeof(int16_t);
    size_t offset = 0;
    while (offset < byteCount) {
        const size_t written = writeAudio(bytes + offset, byteCount - offset);
        if (written == 0) return false;
        offset += written;
    }
    return true;
}

void audioTask(void*) {
    WavFile music;
    std::array<SfxClip, kSfxCacheSize> cache{};
    std::array<SfxVoice, kSfxVoiceCount> voices{};
    size_t nextVoice = 0;

    for (;;) {
        AudioCommand command;
        if (!hasActiveSource(music, voices)) {
            if (!devboxQueueReceive(s_queue, &command, DEVBOX_WAIT_FOREVER)) {
                continue;
            }
            applyCommand(command, music, cache, voices, nextVoice);
        }
        while (devboxQueueReceive(s_queue, &command)) {
            applyCommand(command, music, cache, voices, nextVoice);
        }

        if (!hasActiveSource(music, voices)) {
            s_playing.store(false);
            continue;
        }

        int32_t mix[kChunkSamples] = {};
        size_t mixedSamples = mixMusic(music, mix);
        for (SfxVoice& voice : voices) {
            const size_t count = mixVoice(voice, mix);
            if (count > mixedSamples) mixedSamples = count;
        }

        s_playing.store(hasActiveSource(music, voices) || mixedSamples > 0);
        if (mixedSamples == 0) continue;

        int16_t output[kChunkSamples];
        for (size_t i = 0; i < mixedSamples; ++i) {
            output[i] = clampSample(mix[i]);
        }
        updateAudioVolumeFromPot();
        applyVolumeToBuffer(output, mixedSamples);

        if (!writeMixedChunk(output, mixedSamples)) {
            stopAll(music, voices);
            s_playing.store(false);
        }
    }
}

} // namespace

bool begin() {
    if (s_initialized.load()) return true;

    s_queue = devboxQueueCreate(sizeof(AudioCommand), kQueueLength);
    if (!s_queue) return false;

    const bool started = devboxStartTask(
        audioTask, nullptr, "dbxAudio", kTaskStackBytes,
        kTaskPriority, kTaskCore);
    if (!started) {
        devboxQueueDestroy(s_queue);
        s_queue = nullptr;
        return false;
    }

    s_playing.store(false);
    s_status.store(Status::None);
    s_initialized.store(true);
    return true;
}

void update() {}

bool playSfx(const char* path) {
    AudioCommand command;
    return makePlayCommand(command, path, PlaybackKind::Sfx) && enqueue(command);
}

bool preloadSfx(const char* path) {
    AudioCommand command;
    if (!makePlayCommand(command, path, PlaybackKind::Sfx)) return false;
    command.type = CommandType::Preload;
    return enqueue(command);
}

bool playMusic(const char* path) {
    AudioCommand command;
    return makePlayCommand(command, path, PlaybackKind::Music) && enqueue(command);
}

void stop() {
    AudioCommand command;
    command.type = CommandType::Stop;
    enqueue(command);
}

bool isPlaying() {
    return s_playing.load();
}

Status status() {
    return s_status.load();
}

const char* statusText() {
    switch (status()) {
        case Status::FileOpenFailed: return "SFX: file open failed";
        case Status::InvalidWav: return "SFX: invalid WAV";
        case Status::CacheFull: return "SFX: cache full";
        case Status::OutOfMemory: return "SFX: out of memory";
        case Status::ReadFailed: return "SFX: SD read failed";
        case Status::SfxCached: return "SFX: cached";
        case Status::SfxPlaying: return "SFX: playing";
        case Status::None: return "";
    }
    return "SFX: unknown status";
}

} // namespace dbx::Audio
