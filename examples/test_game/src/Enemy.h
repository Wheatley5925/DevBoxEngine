#pragma once

#include <DevBoxEngine.h>

class Enemy {
public:
    void setPosition(int x, int y);

    bool CheckCollision(int other_x, int other_y, 
        const int other_w, const int other_h);

    void update(float dt);
    void draw(dbx::Renderer& r, const dbx::AtlasAsset& atlas) const;

    int x() const { return m_x; }
    int y() const { return m_y; }
    
    const int w() const { return m_w; }
    const int h() const { return m_h; }

    bool active() const { return m_active; }

    int getHP() const { return m_hp; }
    void setHP(int new_hp);

private:
    int currentFrame() const;

private:
    int m_x = 220;
    int m_y = 56;

    int m_hp = 1;
    bool m_active = true;

    const int m_w = 32;
    const int m_h = 15;

    float m_age = 0.0f;
    float m_waveSpeed = 1.0f;

    float m_animTimer = 0.0f;
    int m_animFrame = 0;
};
