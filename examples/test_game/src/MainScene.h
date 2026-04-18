#pragma once

#include <DevBoxEngine.h>
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
    dbx::BitmapAsset m_stars;
    dbx::BitmapAsset m_planet;
    dbx::BitmapAsset m_laser;
    Player m_player;
};
