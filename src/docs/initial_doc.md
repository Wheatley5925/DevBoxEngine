# DevBoxEngine Working Documentation

This document is the initial design documentation for the engine layer built on top of `DevBoxSDK`.

It is intentionally practical rather than polished. Its purpose is to define architecture, public responsibilities, and working rules early, so the code and docs evolve together.

---

# 1. Architecture

## 1.1 Goal

`DevBoxSDK` is the platform layer.

It contains direct access to:

* display
* buttons
* audio
* SD card
* OTA / boot behavior

`DevBoxEngine` is the game-framework layer built on top of `DevBoxSDK`.

Its purpose is to let a game developer write code in terms of:

* scenes
* update/draw
* renderer
* input
* assets
* audio playback
* system actions

and not in terms of:

* GPIO
* raw framebuffer details
* direct SD reads in gameplay code
* low-level OTA logic
* raw button indices everywhere

## 1.2 Layer split

```text
Game code
  -> DevBoxEngine
      -> DevBoxSDK
          -> hardware
```

### DevBoxSDK responsibilities

* hardware initialization
* framebuffer implementation
* display transfer
* physical button polling
* audio device startup / low-level playback helpers
* SD initialization and low-level file access
* boot / OTA utilities

### DevBoxEngine responsibilities

* game loop orchestration
* scene lifecycle
* game-facing renderer API
* input snapshot abstraction
* asset loading abstraction
* audio abstraction
* system actions abstraction
* later: asset caching, atlas support, async loading, animation helpers

## 1.3 Ownership rules

These ownership rules are important.

### Display

Only the renderer layer presents frames to the display.
Game code must not call low-level display transfer functions directly.

### Input

Only the input layer polls raw button state.
Game code works with high-level queries such as:

* held
* pressed
* released

### Audio

Only the audio layer talks to the low-level playback implementation.
Game code asks for:

* play sound effect
* play music
* stop music

### Storage / assets

Only storage / asset layers read files.
Game code requests assets rather than opening SD files directly during gameplay.

### System actions

Only the system layer performs reboot / return-to-OS / OTA-related actions.
Game code requests those actions.

---

# 2. Frame lifecycle

## 2.1 Engine loop

The engine owns the per-frame flow.

The intended order for a frame is:

1. calculate `dt`
2. update input snapshot
3. update engine services
4. apply pending scene switch
5. run active scene update
6. run active scene draw
7. present the frame
8. execute deferred system actions if needed

## 2.2 Why this order

### Input before update

Game logic should read the newest input state for the current frame.

### Scene switching at a controlled point

A scene should not destroy or replace itself mid-update in an uncontrolled way.
A scene change is requested first, then applied by the engine at a safe point.

### Draw after update

The rendered frame should reflect the newly updated state.

### Present once

Only one place in the engine should flush the framebuffer to hardware.

### Deferred system actions

Actions such as `returnToOS()` should happen after the frame has had a chance to render an exit message or final state.

## 2.3 Time rules

The engine measures frame delta time in seconds.

`dt` may be clamped in order to avoid very large stalls causing broken gameplay behavior.

Example policy:

* normal frame: actual measured `dt`
* large stall: clamp to a maximum such as `0.05f`

This keeps gameplay stable after temporary stalls.

---

# 3. Scene lifecycle

## 3.1 What a scene is

A scene is a self-contained game state.

Examples:

* main menu
* level
* pause menu
* settings screen
* cutscene
* loading screen

## 3.2 Scene API

A scene is expected to provide:

* `onEnter()`
* `onExit()`
* `update(float dt)`
* `draw(Renderer& r)`

## 3.3 Meaning of each function

### `onEnter()`

Called once when the scene becomes active.
Use it for:

* initializing scene-local state
* requesting scene-local assets
* resetting timers

### `onExit()`

Called once before the scene is replaced.
Use it for:

* stopping scene-local actions
* releasing scene-local assets if needed
* saving scene-local progress if needed

### `update(float dt)`

Contains scene logic.
Use it for:

* movement
* collision
* animation state changes
* UI logic
* reading high-level input
* requesting scene changes
* requesting audio or system actions

### `draw(Renderer& r)`

Contains scene drawing commands only.
Use it for:

* clearing the frame
* drawing sprites
* drawing text
* drawing UI

Avoid doing heavy game logic or file I/O here.

## 3.4 Scene switching rule

Scenes should not directly replace the current active scene in the middle of execution.

Instead:

* a scene requests a change
* the engine stores the next scene pointer/reference
* the engine performs the switch at a safe point

This keeps state transitions predictable.

---

# 4. Core modules

## 4.1 `core/Types.h`

Purpose:

* shared small types used across the engine

Expected contents:

* vector types
* rectangle type
* button enum
* small screen constants if needed

It should remain small and stable.

## 4.2 `core/Scene.h`

Purpose:

* define the scene contract used by game code

It should stay independent of low-level hardware details.

## 4.3 `core/Engine.h`

Purpose:

* define the public control surface of the engine

Expected responsibilities:

* `begin()`
* `tick()`
* `requestScene()`
* frame count and delta time accessors

## 4.4 `core/Engine.cpp`

Purpose:

* implement the actual frame orchestration
* startup engine subsystems in the correct order
* own the scene transition policy
* own end-of-frame present behavior

This file should not contain game-specific logic.

---

# 5. Engine API contract

This section defines what each major public module is supposed to mean.

It is a contract, not a full implementation description.

## 5.1 Engine

### Purpose

Own the application/game loop and scene control.

### Public responsibilities

* initialize engine and required platform services
* run one frame
* manage scene transitions
* provide frame timing data

### Game code may

* call `begin()`
* call `tick()` in `loop()`
* request scene changes through engine-facing API

### Game code should not

* reimplement its own top-level frame loop logic on top of the engine
* manually call low-level display present functions

## 5.2 Scene

### Purpose

Represent one self-contained game state.

### Game code may

* derive its own scenes
* store scene-local state
* use `update()` and `draw()` to implement behavior

### Game code should not

* directly initialize hardware
* directly perform OTA partition switching

## 5.3 Renderer

### Purpose

Provide a game-facing drawing API.

### V1 target responsibilities

* clear screen
* draw bitmap
* draw text
* draw rectangle
* present

### Future responsibilities

* atlas drawing
* clipped blitting
* animation helpers
* camera transforms
* dirty rectangle support

### Game code may

* issue draw commands through renderer

### Game code should not

* manipulate the raw framebuffer format directly
* call hardware display transfer functions directly

## 5.4 Input

### Purpose

Convert raw button polling into frame-based input state.

### V1 target responsibilities

* begin
* update
* `held()`
* `pressed()`
* `released()`

### Game code may

* read logical button states

### Game code should not

* use raw physical button indices directly if an engine input API exists

## 5.5 Audio

### Purpose

Provide game-facing sound playback requests.

### V1 target responsibilities

* initialize audio wrapper
* play sound effect

### Future responsibilities

* queued commands
* streamed music
* channel management
* volume groups

### Game code may

* request playback of sounds/music

### Game code should not

* directly depend on low-level audio playback implementation

## 5.6 Assets

### Purpose

Load and represent game assets in engine-friendly form.

### V1 target responsibilities

* load raw bitmap assets
* unload assets

### Future responsibilities

* atlas loading
* animation clips
* caching
* async loading
* package-based loading

### Game code may

* request assets through the asset API

### Game code should not

* scatter direct SD file loading throughout scene logic

## 5.7 Storage

### Purpose

Own generic file and storage-related services.

### V1 target responsibilities

* startup placeholder
* future safe point for async file operations

### Future responsibilities

* queued file requests
* async reads/writes
* save data helpers

## 5.8 System

### Purpose

Own engine-level system actions that affect the application lifecycle.

### V1 target responsibilities

* `requestReturnToOS()`
* `returnToOS()`

### Rule

Potentially disruptive actions should usually be requested first and executed by the engine at a safe point.

---

# 6. Public coding rules

These rules define how game code should interact with the engine.

## 6.1 Allowed style

Game code should mostly do this:

* read input via engine input API
* update scene-local state
* draw through renderer
* request sound playback through audio API
* request scene/system changes through engine/system API

## 6.2 Forbidden style

Game code should avoid:

* direct framebuffer manipulation as a normal workflow
* direct SD file reads inside active gameplay update unless explicitly allowed later
* direct OTA / reboot logic in arbitrary scene code
* direct calls to low-level display present functions
* direct dependency on raw hardware details if an engine wrapper exists

## 6.3 Practical examples

### Minimal application structure

A minimal application using the engine should look conceptually like this:

```cpp
#include <DevBoxEngine.h>

class MenuScene : public dbx::Scene {
public:
    void onEnter() override {
        // initialize scene-local state here
    }

    void update(float dt) override {
        // update logic here
    }

    void draw(dbx::Renderer& r) override {
        r.clear(0);
        r.drawText(10, 20, "Menu");
    }
};

static dbx::Engine engine;
static MenuScene menu;

void setup() {
    engine.begin(&menu);
}

void loop() {
    engine.tick();
}
```

### Reading input in a scene

The intended scene-side input style is:

```cpp
void PlayerScene::update(float dt) {
    if (dbx::Input::held(dbx::Button::Left)) {
        playerX -= 80.0f * dt;
    }

    if (dbx::Input::pressed(dbx::Button::A)) {
        dbx::Audio::playSfx("/sfx/jump.wav");
    }
}
```

This keeps raw button indices and low-level button polling out of gameplay code.

### Requesting return to OS

Returning to the OS should normally be requested rather than executed immediately inside arbitrary game logic.

```cpp
void PauseScene::update(float dt) {
    if (dbx::Input::pressed(dbx::Button::Start)) {
        dbx::System::requestReturnToOS();
    }
}

void PauseScene::draw(dbx::Renderer& r) {
    r.clear(0);
    r.drawText(60, 60, "Exiting...");
}
```

The engine can then execute the actual OTA/boot switch at a safe point after presentation.

### Scene switching example

A scene should request a switch instead of replacing itself in the middle of execution.

```cpp
void MenuScene::update(float dt) {
    if (dbx::Input::pressed(dbx::Button::A)) {
        engine->requestScene(&gameScene);
    }
}
```

This keeps lifecycle transitions predictable and makes future loading/fade transitions easier.

---

# 7. Asset pipeline design

## 7.1 Principle

Source asset formats are not necessarily runtime formats.

The intended pipeline is:

```text
source assets -> asset compiler -> runtime assets -> engine
```

## 7.2 Source formats

Expected authoring/input formats may include:

* PNG
* GIF
* WAV / MP3
* MP4
* JSON / TMX / text metadata

## 7.3 Runtime formats

Runtime formats should be chosen for the engine and device, not for convenience of desktop tools.

Initial direction:

* prepacked raw bitmaps for graphics
* simple animation data for frame sequences
* simple audio format for playback/streaming
* later package file/index format for grouped assets

## 7.4 Sprite atlas policy

Do not load individual sprite files from SD during active gameplay if avoidable.

Preferred policy:

* pack many related sprites into one atlas
* load one or a few atlases for the current scene/level
* render by slicing source rectangles from the loaded atlas

Examples:

* `common` atlas for UI and common effects
* `level_01` atlas for the current level
* optional specialized atlas for boss/content-specific assets

## 7.5 Memory policy

Do not keep all assets for the whole game in RAM.

Preferred policy:

* keep currently needed atlases in RAM
* unload content when leaving a scene/level
* stream only the data that is appropriate to stream

---

# 8. Memory and performance rules

## 8.1 Keep in RAM

Prefer RAM residency for:

* currently visible sprites
* UI assets used every frame
* tile atlases for the active level
* short sound effects if memory allows

## 8.2 Stream or load on demand

Prefer streaming or staged loading for:

* long music tracks
* cutscene/video-like content
* large level-specific data loaded on scene enter
* save/load operations

## 8.3 Avoid during active gameplay

Avoid these in the normal gameplay hot path:

* many tiny SD reads
* runtime decoding of heavy source formats
* large transient allocations each frame
* uncontrolled scene replacement in the middle of logic

---

# 9. Planned implementation order

Recommended order of development:

1. core

   * `Types`
   * `Scene`
   * `Engine`
2. input
3. renderer
4. system
5. raw asset loading
6. sprite atlas support
7. animation helpers
8. audio improvements
9. async loading/storage
10. desktop simulator and asset compiler

---

# 10. Current status

This document reflects the design direction currently chosen:

* `DevBoxSDK` remains the hardware/platform layer
* `DevBoxEngine` is a clean abstraction layer above it
* scenes are the primary unit of game state
* the engine owns frame orchestration
* disruptive system actions are deferred to safe points
* asset formats should be designed for runtime efficiency, not raw authoring convenience

This is a working document and should evolve with the code.
