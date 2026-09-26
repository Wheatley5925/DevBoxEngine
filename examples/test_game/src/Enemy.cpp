#include "Enemy.h"
#include <cmath>
using namespace dbx;

static constexpr float IDLE_FRAME_TIME = 0.5f;

void Enemy::setPosition(int x, int y) {
    m_x = x;
    m_y = y;
}

void Enemy::update(float dt) {
    m_age += dt;

    m_y = 65 + static_cast<int>(38.5f * std::sin(m_age * m_waveSpeed)); 

    m_animTimer += dt;
    while (m_animTimer >= IDLE_FRAME_TIME) {
        m_animTimer -= IDLE_FRAME_TIME;
        m_animFrame ^= 1;
    }
}

int Enemy::currentFrame() const {
    return (m_animFrame == 0) ? 0 : 1;
}

void Enemy::draw(Renderer& r, const AtlasAsset& atlasAsset) const {
    if (!atlasAsset.valid()) return;

    SpriteAtlas enemyAtlas = atlasAsset.view();
    const int frameIndex = currentFrame();

    if (frameIndex < 0 || frameIndex >= enemyAtlas.frameCount) return;

    const SpriteFrame& frame = enemyAtlas.frames[frameIndex];
    const int drawX = m_x - frame.originX;
    const int drawY = m_y - frame.originY;

    r.drawSprite(enemyAtlas, frameIndex, drawX, drawY, true,
                 atlasAsset.transparentColor);
}
