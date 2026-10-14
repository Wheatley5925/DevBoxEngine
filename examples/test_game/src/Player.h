#pragma once

#include <DevBoxEngine.h>

static constexpr float COOLDOWN_TIME = 0.33f;
static constexpr int MAX_LASERS = 3;

struct gameobject {
    int  m_x = 0;
    int  m_y = 0;
    bool active = false;
};

class Player {
public:
	void setPosition(int x, int y);
	void setHP(int hp);

	void update(float dt);
	void draw(dbx::Renderer& r, const dbx::AtlasAsset& atlas, const dbx::BitmapAsset& laser, const dbx::AtlasAsset& shoot_fxAtlasAsset) const;

	int hp() const { return m_hp; }
	int x()  const { return m_x;  }
	int y()  const { return m_y;  }

    const gameobject* lasers() const { return m_lasers; } 
    void deactivateLaser(int index);

private:
    int currentFrame() const;
    void clampPosition();
    void tryShoot();

private:
    int m_x = 25;
    int m_y = 50;

    const int m_w = 42;
    const int m_h = 21;

    int min_x = 0;
    int min_y = 12;
    int max_x = 212;
    int max_y = 108;

    int m_hp = 10;
    int m_speed = 150;

    gameobject m_lasers[MAX_LASERS];
    int m_laser_cnt = 0;
    int frames_since_shot = 5;

    dbx::Vec2i m_laser_pos;

    float m_cooldownTimer = 0.0f;
    float m_animTimer = 0.0f;
    int m_animFrame = 0;
};
