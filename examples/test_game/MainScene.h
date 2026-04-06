#pragma once

#include <DevBoxEngine.h>

class MainScene : public dbx::Scene {
public:
    void onEnter() override;
    void onExit() override;
    void update(float dt) override;
    void draw(dbx::Renderer& r) override;

private:
    dbx::AtlasAsset m_playerAtlas;

    int m_frame0 = -1;
    int m_frame1 = -1;

    float m_animTimer = 0.0f;
    int m_currentFrame = 0;
};
