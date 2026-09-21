#include "Player.h"

using namespace dbx;

static constexpr float IDLE_FRAME_TIME = 0.5f;
static constexpr const char* LASER_SOUND_PATH =
    "/sdcard/apps/test_game/assets/main/laserShoot.wav";

void Player::clampPosition() {
    m_x = (m_x > max_x) ? max_x : m_x;
    m_x = (m_x < min_x) ? min_x	: m_x;

    m_y = (m_y > max_y) ? max_y : m_y;
    m_y = (m_y < min_y)	? min_y : m_y;
}

void Player::setPosition(int x, int y) {
    m_x = x;
    m_y = y;
    Player::clampPosition();
}

void Player::setHP(int hp) {
    m_hp = hp;
}

void Player::tryShoot() {
    if (m_laser_cnt > 0) return;

    Audio::playSfx(LASER_SOUND_PATH);

    frames_since_shot = 0;

    m_laser_cnt++;
    m_laser_pos.x = m_x + 41;
    m_laser_pos.y = m_y + 11;
}

void Player::update(float dt) {
    int dx = 0;
    int dy = 0;

    if (Input::held(Button::Left))  dx -= 1;
    if (Input::held(Button::Right)) dx += 1;
    if (Input::held(Button::Up))    dy -= 1;
    if (Input::held(Button::Down))  dy += 1;

    if (Input::pressed(Button::B)) tryShoot();

    if (m_laser_cnt > 0) {
	    m_laser_pos.x += 4;
    	if (m_laser_pos.x > 256) m_laser_cnt = 0;
    }

    m_x += static_cast<int>(dx * m_speed * dt);
    m_y += static_cast<int>(dy * m_speed * dt);

    Player::clampPosition();

    frames_since_shot++;
    m_animTimer += dt;
    while (m_animTimer >= IDLE_FRAME_TIME) {
        m_animTimer -= IDLE_FRAME_TIME;
        m_animFrame ^= 1;
    }
}

int Player::currentFrame() const {
    return (m_animFrame == 0) ? 0 : 1;
}

void Player::draw(Renderer& r, const AtlasAsset& atlasAsset, const BitmapAsset& laser, const BitmapAsset& shoot_fx) const {
    if (!atlasAsset.valid()) return;

    SpriteAtlas atlas = atlasAsset.view();
    const int frameIndex = currentFrame();

    if (frameIndex < 0 || frameIndex >= atlas.frameCount) return;

    const SpriteFrame& fr = atlas.frames[frameIndex];
    const int drawX = m_x - fr.originX;
    const int drawY = m_y - fr.originY;
    
    if (m_laser_cnt > 0) {
    	r.drawBitmap(laser.view(), m_laser_pos.x, m_laser_pos.y, false, 0);
    }

    if (frames_since_shot < 5) {
        r.drawBitmap(shoot_fx.view(), drawX + 42, drawY + 7, false, 0);
    }

    r.drawSprite(atlas, frameIndex, drawX, drawY, true, atlasAsset.transparentColor);
}
