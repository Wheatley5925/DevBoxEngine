#pragma once

namespace dbx {

class Renderer;

class Scene {
public:
    virtual ~Scene() = default;

    // Called once when the scene becomes active
    virtual void onEnter() {}

    // Called once right before the scene is replaced
    virtual void onExit() {}

    // Per-frame game logic
    virtual void update(float dt) = 0;

    // Per-frame drawing
    virtual void draw(Renderer& r) = 0;
};

} // namespace dbx
