#include "Engine.h"

#include <DevBoxSDK.h>

#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX
#include <array>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#endif

#include "Scene.h"

#include "../gfx/Renderer.h"
#include "../input/Input.h"
#include "../audio/Audio.h"
#include "../assets/Assets.h"
#include "../system/Storage.h"
#include "../system/System.h"

namespace dbx {

static Renderer g_renderer;
constexpr uint32_t kFrameTimeUs = 1000000 / 60;

#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX
namespace {

enum TimingPart : size_t {
    FrameInterval, Pace, InputWall, InputCpu, OtherServices,
    SceneUpdate, SceneDraw, Present, PresentCpu, PartCount
};

uint64_t threadCpuMicros() {
    timespec time{};
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time);
    return static_cast<uint64_t>(time.tv_sec) * 1000000u +
           static_cast<uint64_t>(time.tv_nsec) / 1000u;
}

struct FrameTimings {
    std::array<uint64_t, PartCount> sums{};
    std::array<uint32_t, PartCount> worst{};
    uint32_t samples = 0;
    uint32_t slowFrames = 0;
    uint32_t sampleStart = 0;

    void record(const std::array<uint32_t, PartCount>& times, uint32_t now) {
        if (sampleStart == 0) sampleStart = now;
        for (size_t i = 0; i < PartCount; ++i) {
            sums[i] += times[i];
            if (times[i] > worst[i]) worst[i] = times[i];
        }
        ++samples;
        if (times[FrameInterval] > 20000) ++slowFrames;
        if (now - sampleStart < 1000000) return;

        std::printf(
            "Frame timing ms (avg/max): interval %.2f/%.2f, "
            "pace %.2f/%.2f, input %.2f/%.2f "
            "(CPU %.2f/%.2f), other services %.2f/%.2f, "
            "update %.2f/%.2f, draw %.2f/%.2f, "
            "present %.2f/%.2f (CPU %.2f/%.2f), >20ms %u/%u\n",
            sums[FrameInterval] / (samples * 1000.0), worst[FrameInterval] / 1000.0,
            sums[Pace] / (samples * 1000.0), worst[Pace] / 1000.0,
            sums[InputWall] / (samples * 1000.0), worst[InputWall] / 1000.0,
            sums[InputCpu] / (samples * 1000.0), worst[InputCpu] / 1000.0,
            sums[OtherServices] / (samples * 1000.0), worst[OtherServices] / 1000.0,
            sums[SceneUpdate] / (samples * 1000.0), worst[SceneUpdate] / 1000.0,
            sums[SceneDraw] / (samples * 1000.0), worst[SceneDraw] / 1000.0,
            sums[Present] / (samples * 1000.0), worst[Present] / 1000.0,
            sums[PresentCpu] / (samples * 1000.0), worst[PresentCpu] / 1000.0,
            slowFrames, samples);
        *this = FrameTimings{};
        sampleStart = now;
    }
};

} // namespace
#endif

bool Engine::begin(Scene* firstScene) {
    if (m_started) {
        return true;
    }

    initDebugOutput();
    initSD();
    initAudio();
    const bool displayOk = initDisplay();
    initButtons();

    Input::begin();
    Audio::begin();
    Assets::begin();
    Storage::begin();
    System::begin();

    m_scene = firstScene;
    m_nextScene = nullptr;
    m_lastMicros = devboxMicros();
    m_frameCount = 0;
    m_dt = 1.0f / 60.0f;
    m_started = true;

    if (m_scene) {
        m_scene->onEnter();
    }

    return displayOk;
}

void Engine::requestScene(Scene* nextScene) {
    m_nextScene = nextScene;
}

void Engine::switchSceneIfNeeded() {
    if (!m_nextScene) {
        return;
    }

    if (m_scene) {
        m_scene->onExit();
    }

    m_scene = m_nextScene;
    m_nextScene = nullptr;

    if (m_scene) {
        m_scene->onEnter();
    }
}

void Engine::tick() {
    if (!m_started) {
        return;
    }

#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX
    static const bool profileFrames = std::getenv("DEVBOX_PROFILE_FRAMES") != nullptr;
    static FrameTimings timings;
    const uint32_t tickStart = profileFrames ? devboxMicros() : 0;
    const uint32_t previousFrameStart = m_lastMicros;
#endif
    uint32_t now = devboxMicros();
    uint32_t deltaUs = now - m_lastMicros;
    if (deltaUs < kFrameTimeUs) {
        devboxDelayMicros(kFrameTimeUs - deltaUs);
        now = devboxMicros();
        deltaUs = now - m_lastMicros;
    }
    m_lastMicros = now;

    m_dt = deltaUs / 1000000.0f;
    if (m_dt > 0.05f) {
        m_dt = 0.05f;
    }

    if (devboxApplicationPaused()) {
        g_renderer.present();
        return;
    }

#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX
    const uint32_t pacedAt = now;
    const uint64_t inputCpuStart = profileFrames ? threadCpuMicros() : 0;
#endif
    Input::update();
#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX
    const uint64_t inputCpuEnd = profileFrames ? threadCpuMicros() : 0;
    const uint32_t inputAt = profileFrames ? devboxMicros() : 0;
#endif
    Audio::update();
    Assets::update();
    Storage::update();
    System::update();

    switchSceneIfNeeded();

#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX
    const uint32_t servicesAt = profileFrames ? devboxMicros() : 0;
    uint32_t updatedAt = 0;
    uint32_t drawnAt = 0;
#endif

    if (m_scene) {
        m_scene->update(m_dt);
#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX
        updatedAt = profileFrames ? devboxMicros() : 0;
#endif
        m_scene->draw(g_renderer);
#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX
        drawnAt = profileFrames ? devboxMicros() : 0;
#endif
    }

#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX
    const uint64_t presentCpuStart = profileFrames ? threadCpuMicros() : 0;
#endif
    g_renderer.present();

#if DEVBOX_TARGET == DEVBOX_TARGET_LINUX
    if (profileFrames && m_scene) {
        const uint32_t presentedAt = devboxMicros();
        const uint64_t presentCpuEnd = threadCpuMicros();
        timings.record({pacedAt - previousFrameStart, pacedAt - tickStart,
                        inputAt - pacedAt,
                        static_cast<uint32_t>(inputCpuEnd - inputCpuStart),
                        servicesAt - inputAt, updatedAt - servicesAt,
                        drawnAt - updatedAt, presentedAt - drawnAt,
                        static_cast<uint32_t>(presentCpuEnd - presentCpuStart)},
                       presentedAt);
    }
#endif

    if (dbx::System::returnToOSRequested()) {
        dbx::System::returnToOS();
    }

    ++m_frameCount;
}

} // namespace dbx
