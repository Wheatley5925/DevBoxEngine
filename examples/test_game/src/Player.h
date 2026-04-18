#pragma once

#include <DevBoxEngine.h>

class Player {
public:
	void setPosition(int x, int y);
	void setHP(int hp);

	void update(float dt);
	void draw(dbx::Renderer& r, const dbx::AtlasAsset& atlas, const dbx::BitmapAsset& laser) const;

	int hp() const { return m_hp; }
	int x()  const { return m_x;  }
	int y()  const { return m_y;  }

private:
    int currentFrame() const;
    void clampPosition();
    void tryShoot();

private:
    int m_x = 25;
    int m_y = 50;
    
    int min_x = 0;
    int min_y = 12;
    int max_x = 212;
    int max_y = 108;

    int m_hp = 10;
    int m_speed = 150;

    int m_laser_cnt = 0;

    dbx::Vec2i m_laser_pos;

    float m_animTimer = 0.0f;
    int m_animFrame = 0;
};
