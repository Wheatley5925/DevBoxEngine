#pragma once

#include <DevBoxEngine.h>
#include "Enemy.h"
#include "Player.h"

class MainScene : public dbx::Scene {
public:
    void onEnter() override;
    void onExit() override;
    void update(float dt) override;
    void draw(dbx::Renderer& r) override;

private:
    dbx::AtlasAsset m_playerAtlas;
    dbx::AtlasAsset m_hpAtlas;
    dbx::AtlasAsset m_enemyAtlas;
    dbx::BitmapAsset m_stars;
    dbx::BitmapAsset m_planet;
    dbx::BitmapAsset m_laser;
    dbx::AtlasAsset m_shoot_fxAtlas;
    Player m_player;
    Enemy m_enemy;
};
