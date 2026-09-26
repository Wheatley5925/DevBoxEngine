#include "MainScene.h"

using namespace dbx;

static constexpr const char* PLAYER_ATLAS_PATH =
    "/sdcard/apps/test_game/assets/main/player.atlas.atl4";
static constexpr const char* HP_ATLAS_PATH =
    "/sdcard/apps/test_game/assets/main/hp.atlas.atl4";
static constexpr const char* STARS_PATH =
    "/sdcard/apps/test_game/assets/main/stars.spr4";
static constexpr const char* PLANET_PATH =
    "/sdcard/apps/test_game/assets/main/planet.spr4";
static constexpr const char* LASER_PATH = 
    "/sdcard/apps/test_game/assets/main/laser.spr4";
static constexpr const char* SHOOT_FX_ATLAS_PATH =
    "/sdcard/apps/test_game/assets/main/shoot_fx.atlas.atl4";
static constexpr const char* MUSIC_PATH = 
    "/sdcard/apps/test_game/assets/music/Artificial_Intelegence.wav";
static constexpr const char* LASER_SOUND_PATH =
    "/sdcard/apps/test_game/assets/main/laserShoot.wav";

static constexpr const char* ENEMY_ATLAS_PATH =
    "/sdcard/apps/test_game/assets/main/enemy.atlas.atl4";

void MainScene::onEnter() {
    m_playerAtlas = Assets::loadAtlas(PLAYER_ATLAS_PATH);
    m_hpAtlas = Assets::loadAtlas(HP_ATLAS_PATH);
    m_stars = Assets::loadBitmap(STARS_PATH);
    m_planet = Assets::loadBitmap(PLANET_PATH);
    m_enemyAtlas = Assets::loadAtlas(ENEMY_ATLAS_PATH);
    m_laser = Assets::loadBitmap(LASER_PATH);
    m_shoot_fxAtlas = Assets::loadAtlas(SHOOT_FX_ATLAS_PATH);

    m_player.setPosition(120, 56);
    m_player.setHP(5);
    m_enemy.setPosition(220, 56);
    Audio::preloadSfx(LASER_SOUND_PATH);
    Audio::playMusic(MUSIC_PATH);
}

void MainScene::onExit() {
    Assets::unload(m_playerAtlas);
    Assets::unload(m_hpAtlas);
    Assets::unload(m_enemyAtlas);
    Assets::unload(m_stars);
    Assets::unload(m_planet);
    Assets::unload(m_laser);
    Assets::unload(m_shoot_fxAtlas);
}

int stars_x = 0;
float planet_x = 300;

void MainScene::update(float dt) {
    stars_x -= 1;
    planet_x -= 0.5f;

    if (stars_x < -255) 
	    stars_x = 0;
    if (planet_x < -700)
	    planet_x = 300;

    m_player.update(dt);
    m_enemy.update(dt);
}

void MainScene::draw(Renderer& r) {
    r.clear(0);

    if (!m_playerAtlas.valid()) {
        r.drawText(8, 16, "player atlas load failed", 15);
        return;
    }


    r.drawBitmap(m_stars.view(), stars_x, 0, true, 0);
    r.drawBitmap(m_stars.view(), stars_x + 256, 0, true, 0);

    r.drawBitmap(m_planet.view(), static_cast<int>(planet_x), -10, true, 0);
    
    m_player.draw(r, m_playerAtlas, m_laser, m_shoot_fxAtlas);

    m_enemy.draw(r, m_enemyAtlas);

    SpriteAtlas atlas = m_hpAtlas.view();
    for (int i = 0; i < 10; i++) {
	if (i < 9)
		r.drawSprite(atlas, (m_player.hp() < i + 1) ? 0 : 1, 5 * i, 0, true, m_hpAtlas.transparentColor);
	else
		r.drawSprite(atlas, (m_player.hp() < i + 1) ? 2 : 3, 5 * i, 0, true, m_hpAtlas.transparentColor);
    }

}
