#include "start_state.hpp"
#include "world/world_system.hpp"
#include <iostream>
#include "core/ecs_registry.hpp"
#include "help_state.hpp"

#include "play_state.hpp"
#include "serialization/registry_serializer.hpp"

HelpState* HelpState::instance() {
    static HelpState instance;
    return &instance;
}

void HelpState::init(WorldSystem* game) {
    background = createBackground(TEXTURE_ASSET_ID::HELP_BACKGROUND);
    registry.uiElements.emplace(background);

    renderer = game->get_renderer();
    textRenderer = game->get_text_renderer();
    shadowRenderer = game->get_shadow_renderer();
}

void HelpState::pause() {
  printf("Help State paused\n");
}

void HelpState::resume() {
  printf("Help State resumed\n");
}

void HelpState::cleanup(WorldSystem* game) {
    std::string stateName = typeid(*this).name();
    // Debug: Check if the registry is clean after step
    // printf("Registry state before cleanup:\n");
    // printf("Number of entities in registry: %zu\n", registry.get_entities().size());

    registry.get_entities().clear();
    registry.clear_all_components();
    background = Entity();
    buttons.clear();

    // printf("Registry state after cleanup:\n");
    // printf("Number of entities in registry: %zu\n", registry.get_entities().size());
}

void HelpState::reset(WorldSystem *game) {

}

void HelpState::handle_key_input(WorldSystem *game, int key, int scancode, int action, int mods) {
    // quit directly, technically there shouldnt be anything to save in start
    if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(game->get_window(), GLFW_TRUE);
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        // printf("SPACE PRESSED in start state: switching to play screen\n");
        game->change_state(PlayState::instance());
    }
}

void HelpState::handle_mouse_input(WorldSystem* game, double xpos, double ypos) {
}

void HelpState::handle_mouse_movement(WorldSystem* game, double xpos, double ypos){
    
}

void HelpState::update(WorldSystem* game, float elapsed_ms) {
    // Update start screen logic (e.g., animations)
}

void HelpState::draw(WorldSystem* game, float elapsed_ms_since_last_update) {
    renderer->draw(elapsed_ms_since_last_update, show_pause_menu);
    if (show_exit_menu) {
        promptSaveOnExit(game);
    }
    glfwSwapBuffers(game->get_window());
}