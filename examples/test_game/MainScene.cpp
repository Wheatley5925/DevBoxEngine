#include "MainScene.h"
#include <SD_MMC.h>
#include <Arduino.h>
using namespace dbx;

static constexpr const char* PLAYER_ATLAS_PATH = "/sdcard/apps/test_game/assets/main/player.atlas.atl4";
static constexpr float IDLE_FRAME_TIME = 0.5f; // 500 ms

void MainScene::onEnter() {
    Serial.begin(115200);

    m_playerAtlas = Assets::loadAtlas(PLAYER_ATLAS_PATH);

    // Temporary: use frame indices directly.
    // Assumes frame 0 = player0, frame 1 = player1.
    m_frame0 = 0;
    m_frame1 = 1;

    m_animTimer = 0.0f;
    m_currentFrame = 0;
}

void MainScene::onExit() {
    Assets::unload(m_playerAtlas);
}

void MainScene::update(float dt) {
    if (!m_playerAtlas.valid()) {
        return;
    }

    m_animTimer += dt;
    while (m_animTimer >= IDLE_FRAME_TIME) {
        m_animTimer -= IDLE_FRAME_TIME;
        m_currentFrame ^= 1;
    }
}

void MainScene::draw(Renderer& r) {
    r.clear(0);

    if (!m_playerAtlas.valid()) {
        r.drawText(8, 16, "atlas load failed", 15);
        return;
    }

    const SpriteAtlas atlas = m_playerAtlas.view();
    const int frameIndex = (m_currentFrame == 0) ? m_frame0 : m_frame1;

    if (frameIndex < 0 || frameIndex >= atlas.frameCount) {
        r.drawText(8, 16, "bad frame index", 15);
        return;
    }

    const SpriteFrame& fr = atlas.frames[frameIndex];

    const int x = (256 - fr.w) / 2 - fr.originX;
    const int y = (128 - fr.h) / 2 - fr.originY;

    r.drawSprite(atlas, frameIndex, x, y, false, m_playerAtlas.transparentColor);
}
