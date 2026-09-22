#pragma once
#include "systems/render_system.hpp"
#include <vector>
#include "core/ecs.hpp"
#include "core/components.hpp"
#include "systems/text_renderer.hpp"
#include "systems/shadow_renderer.hpp"

class WorldSystem;

// Using State pattern to improve maintainability
// Abstract template for all states. They will inherit this class and override the functions if needed.

// followed this tutorial on how to manage game states
// http://gamedevgeek.com/tutorials/managing-game-states-in-c/

enum class BUTTON_TYPE {
    START,
    SHOW_STATS,
    ATTACK,
    CHARM,
    INTIMIDATE,
    TYPES_SIZE
};

struct myButton {
    Entity entity;
    vec2 position;
    vec2 size;
    BUTTON_TYPE type;
};

class GameState {
protected:
    Entity background;
    std::vector<myButton> buttons;

public:
    virtual ~GameState() = default;

    virtual void init(WorldSystem* game) = 0;
    virtual void cleanup(WorldSystem* game) = 0;
    virtual void reset(WorldSystem *game) = 0;

    virtual void pause() = 0;
    virtual void resume() = 0;

    Entity player_character;

    virtual void handle_key_input(WorldSystem *game, int key, int scancode, int action, int mods) = 0;
    virtual void handle_mouse_input(WorldSystem* game, double xpos, double ypos) = 0;
    virtual void handle_mouse_movement(WorldSystem* game, double xpos, double ypos) = 0;
    virtual void update(WorldSystem* game, float elapsed_ms) = 0;
    virtual void draw(WorldSystem* game, float elapsed_ms_since_last_update) = 0;
    void drawUIElements(WorldSystem* game, Entity player_entity);

    Entity createBackground(TEXTURE_ASSET_ID texture_id);
    myButton createButton(vec2 position, vec2 size, TEXTURE_ASSET_ID texture_id, BUTTON_TYPE button_type);
    Entity createUIItem(vec2 position, vec2 size, TEXTURE_ASSET_ID texture_id);
    bool is_click_inside_button(const vec2 & mouse_pos, const myButton &button, WorldSystem &game);
    TextRenderer* textRenderer = nullptr;
    RenderSystem* renderer = nullptr;
    ShadowRenderer* shadowRenderer = nullptr;

    void promptSaveOnExit(WorldSystem* game);

    void updateHealthBar(Entity player);
    Entity createHealthBar();
    Entity health_bar;

    static Entity createPlayerProfile();
    Entity player_profile;
    virtual void buryPlayer(Entity player, Room* currentRoom);

    void createHearts();
    void renderHearts(int lives);
    std::vector<Entity> heartEntities;
    int maxLives = 3;

    static Entity createInventory();
    Entity inventory;
    void drawEquipped(Entity player);

    void showStats(const Stats& playerStats, WorldSystem* game, vec2 position);
    myButton showStatsButton;
    myButton hideStatsButton;
    bool show_stats = false;
    Entity stats_box;

    bool show_help_menu = false;
    bool show_pause_menu = false;
    bool show_exit_menu = false;
    bool show_fps = false;
    GLuint boxVAO, boxVBO, boxEBO;

    float UI_tile_x = window_width_px / 50;
    float UI_tile_y = window_height_px / 50;
};

