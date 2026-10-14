#include "Player.h"

using namespace dbx;


static constexpr float PLAYER_IDLE_FRAME_TIME = 0.5f;
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

void Player::deactivateLaser(int index) {
    if (index < 0 || index >= MAX_LASERS || !m_lasers[index].active) 
        return;
    m_lasers[index].active = false;
    m_laser_cnt--;
}

void Player::tryShoot() {
    if (m_laser_cnt >= MAX_LASERS) return;
    if (m_cooldownTimer < COOLDOWN_TIME) return;

    Audio::playSfx(LASER_SOUND_PATH);

    frames_since_shot = 0;
    m_cooldownTimer = 0.0f;

    m_laser_cnt++;
    
    for (int i = 0; i < MAX_LASERS; i++)
        if (m_lasers[i].active == false) {
            m_lasers[i].m_x = m_x + 41;
            m_lasers[i].m_y = m_y + 11;
            m_lasers[i].active = true;
            break;
        }
}

void Player::update(float dt) {
    int dx = 0;
    int dy = 0;

    if (Input::held(Button::Left))  dx -= 1;
    if (Input::held(Button::Right)) dx += 1;
    if (Input::held(Button::Up))    dy -= 1;
    if (Input::held(Button::Down))  dy += 1;

    if (Input::pressed(Button::B)) tryShoot();

    for (int i = 0; i < MAX_LASERS; i++) {
        if (m_lasers[i].active == true) {
            m_lasers[i].m_x += 4;
            if (m_lasers[i].m_x > 256) {
                m_laser_cnt--;
                m_lasers[i].active = false;
            }
        }
    }

    m_x += static_cast<int>(dx * m_speed * dt);
    m_y += static_cast<int>(dy * m_speed * dt);

    Player::clampPosition();

    frames_since_shot++;
    m_cooldownTimer += dt;
    m_animTimer += dt;
    while (m_animTimer >= PLAYER_IDLE_FRAME_TIME) {
        m_animTimer -= PLAYER_IDLE_FRAME_TIME;
        m_animFrame ^= 1;
    }
}

int Player::currentFrame() const {
    return (m_animFrame == 0) ? 0 : 1;
}

void Player::draw(Renderer& r, const AtlasAsset& atlasAsset, const BitmapAsset& laser, const AtlasAsset& shoot_fxAtlasAsset) const {
    if (!atlasAsset.valid()) return;

    SpriteAtlas playerAtlas = atlasAsset.view();
    const int frameIndex = currentFrame();

    if (frameIndex < 0 || frameIndex >= playerAtlas.frameCount) return;

    const SpriteFrame& fr = playerAtlas.frames[frameIndex];
    const int drawX = m_x - fr.originX;
    const int drawY = m_y - fr.originY;

    for (int i = 0; i < MAX_LASERS; i++) {
        if (m_lasers[i].active == false) continue;
        r.drawBitmap(laser.view(), m_lasers[i].m_x, m_lasers[i].m_y, false, 0);
    }

    r.drawSprite(playerAtlas, frameIndex, drawX, drawY, true, atlasAsset.transparentColor);

    SpriteAtlas shoot_fxAtlas = shoot_fxAtlasAsset.view();
    if (frames_since_shot < 6) {
        r.drawSprite(shoot_fxAtlas, (frames_since_shot + 1) / 3, drawX + 41, drawY + 5, true, shoot_fxAtlasAsset.transparentColor);
    }
}
