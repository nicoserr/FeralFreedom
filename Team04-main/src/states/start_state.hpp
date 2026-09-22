#pragma once
#include "core/ecs.hpp"
#include "game_state.hpp"

class StartState : public GameState {
public:
    static StartState* instance();  // Singleton pattern to ensure only one instance

    void init(WorldSystem* game) override;
    void pause() override;
    void resume() override;
    void cleanup(WorldSystem* game) override;
    void reset(WorldSystem *game) override;

    void handle_key_input(WorldSystem *game, int key, int scancode, int action, int mods) override;
    void handle_mouse_input(WorldSystem* game, double xpos, double ypos) override;
    void handle_mouse_movement(WorldSystem* game, double xpos, double ypos) override;
    void update(WorldSystem* game, float elapsed_ms) override;
    void draw(WorldSystem* game, float elapsed_ms_since_last_update) override;
    static Entity createRotatingEarth();

private:
    StartState() = default;
};