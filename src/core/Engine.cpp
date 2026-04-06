#include "Engine.h"

#include <Arduino.h>
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

bool Engine::begin(Scene* firstScene) {
    if (m_started) {
        return true;
    }

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
    m_lastMicros = micros();
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

    const uint32_t now = micros();
    const uint32_t deltaUs = now - m_lastMicros;
    m_lastMicros = now;

    m_dt = deltaUs / 1000000.0f;
    if (m_dt > 0.05f) {
        m_dt = 0.05f;
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
