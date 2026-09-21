#include "Engine.h"

#include <DevBoxSDK.h>

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

    Input::update();
    Audio::update();
    Assets::update();
    Storage::update();
    System::update();

    switchSceneIfNeeded();

    if (m_scene) {
        m_scene->update(m_dt);
        m_scene->draw(g_renderer);
    }

    g_renderer.present();

    if (dbx::System::returnToOSRequested()) {
        dbx::System::returnToOS();
    }

    ++m_frameCount;
}

} // namespace dbx
