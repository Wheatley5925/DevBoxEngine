#pragma once

#include <stdint.h>

namespace dbx {

class Scene;
class Renderer;

class Engine {
public:
    bool begin(Scene* firstScene);
    void tick();

    void requestScene(Scene* nextScene);

    Scene* currentScene() const { return m_scene; }
    float deltaTime() const { return m_dt; }
    uint32_t frameCount() const { return m_frameCount; }

private:
    void switchSceneIfNeeded();

    Scene* m_scene = nullptr;
    Scene* m_nextScene = nullptr;

    uint32_t m_lastMicros = 0;
    uint32_t m_frameCount = 0;
    float m_dt = 1.0f / 60.0f;

    bool m_started = false;
};

} // namespace dbx
