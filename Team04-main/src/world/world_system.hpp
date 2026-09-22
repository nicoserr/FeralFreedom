#pragma once

#include "core/ecs.hpp"
#include "systems/render_system.hpp"
#include "systems/text_renderer.hpp"
#include "systems/shadow_renderer.hpp"
#include <random>
#include "../common.hpp"
#include <vector>
#include "core/components.hpp"
#include "states/game_state.hpp"
#include "systems/levels_rooms_system.hpp"
#include "systems/collision_system.hpp"
#include "systems/visual_effects_system.hpp"
#include "systems/pathfinding_system.hpp"

class WorldSystem {
public:
    WorldSystem();
    // Releases resources
    ~WorldSystem();

    // Create game window
    GLFWwindow *create_window();
    GLFWwindow *get_window() { return window; }

        // Start the game
        void init(RenderSystem* renderer, LevelSystem*, CollisionSystem*, TextRenderer* text_renderer, ShadowRenderer* shadow_renderer, PathFindingSystem* path_finding_system_arg);

        bool step(float elapsed_ms);
    // TODO : remove
        bool isPlayState = false;

        bool is_over() const;
        void cleanup();

        // For state management
        void change_state(GameState* state);
        void push_state(GameState* state);
        void pop_state();
        GameState* get_current_state() const;
        RenderSystem* get_renderer() const { return renderer; }
        LevelSystem* get_level_manager() const {return levelManager; }
        void set_level_manager(LevelSystem* level_system) { levelManager = level_system; }
        CollisionSystem* get_collision_system() const {return collisionManager; }
        TextRenderer* get_text_renderer() const { return text_renderer; }
        ShadowRenderer* get_shadow_renderer() const {return shadow_renderer; }
        VisualEffectsSystem* get_visual_effects_system() const { return visualEffectsSystem; }
        PathFindingSystem* get_path_finding_system() const {return path_finding_system;}
        void restart_game();

    private:
        GLFWwindow* window;
        RenderSystem* renderer;
        LevelSystem* levelManager;
        CollisionSystem* collisionManager;
        VisualEffectsSystem* visualEffectsSystem;
        TextRenderer* text_renderer;
        ShadowRenderer* shadow_renderer;
        PathFindingSystem *path_finding_system;
        std::vector<GameState*> states;

        bool chase_active = false;

        //input callbacks
        void on_key(int key, int, int action, int mod);
        void on_mouse_move(vec2 mouse_position);
        void on_mouse_click(double xpos, double ypos);
        void cleanup_load_save_directory();

    // C++ random number generator
    std::default_random_engine rng;
    std::uniform_real_distribution<float> random_step;
    std::uniform_int_distribution<int> random_angle;
    std::vector<float> angles = {0, M_PI / 4, M_PI / 2, 3 * M_PI / 4, M_PI, 5 * M_PI / 4, 3 * M_PI / 2, 7 * M_PI / 4};
};
