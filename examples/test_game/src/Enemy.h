#pragma once

#include <DevBoxEngine.h>

class Enemy {
public:
    void setPosition(int x, int y);

    void update(float dt);
    void draw(dbx::Renderer& r, const dbx::AtlasAsset& atlas) const;

    int x() const { return m_x; }
    int y() const { return m_y; }

private:
    int currentFrame() const;

private:
    int m_x = 220;
    int m_y = 56;

    float m_age = 0.0f;
    float m_waveSpeed = 1.0f;

    float m_animTimer = 0.0f;
    int m_animFrame = 0;
};
