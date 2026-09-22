#include "start_state.hpp"
#include "world/world_system.hpp"
#include <iostream>

#include "help_state.hpp"
#include "core/ecs_registry.hpp"
#include "play_state.hpp"
#include "serialization/registry_serializer.hpp"

StartState* StartState::instance() {
    static StartState instance;
    return &instance;
}

void StartState::init(WorldSystem* game) {
    background = createBackground(TEXTURE_ASSET_ID::SPACE_BACKGROUND);
    registry.uiElements.emplace(background);
    Entity earth = createRotatingEarth();
    registry.uiElements.emplace(earth);

    renderer = game->get_renderer();
    textRenderer = game->get_text_renderer();
    shadowRenderer = game->get_shadow_renderer();

    myButton startButton = createButton(
        vec2(window_width_px / 2, window_height_px - 100),
        vec2(200, 100),
        TEXTURE_ASSET_ID::START_BUTTON,
        BUTTON_TYPE::START
    );
    buttons.push_back(startButton);
    registry.uiElements.emplace(startButton.entity);

    Entity logo = createUIItem(vec2(window_width_px/2, window_height_px/6), vec2(400, 150), TEXTURE_ASSET_ID::GAME_LOGO);
    registry.uiElements.emplace(logo);
    // printf("Start State initialized\n");
}

void StartState::pause() {
  printf("Start State paused\n");
}

void StartState::resume() {
  printf("Start State resumed\n");
}

void StartState::cleanup(WorldSystem* game) {
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

void StartState::reset(WorldSystem *game) {

}

void StartState::handle_key_input(WorldSystem *game, int key, int scancode, int action, int mods) {
    // quit directly, technically there shouldnt be anything to save in start
    if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(game->get_window(), GLFW_TRUE);
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        // printf("SPACE PRESSED in start state: switching to play screen\n");
        game->change_state(HelpState::instance());
    }
}

void StartState::handle_mouse_input(WorldSystem* game, double xpos, double ypos) {
    // Check if the mouse click is inside any button
    for (const auto& button : buttons) {
        if (is_click_inside_button({xpos,ypos}, button, *game)) {
            // printf("START BUTTON CLICKED: switching to play screen\n");
            game->change_state(HelpState::instance());
            return;
        }
    }
}
void StartState::handle_mouse_movement(WorldSystem* game, double xpos, double ypos){
    
}

void StartState::update(WorldSystem* game, float elapsed_ms) {
    // Update start screen logic (e.g., animations)
}

void StartState::draw(WorldSystem* game, float elapsed_ms_since_last_update) {
    renderer->draw(elapsed_ms_since_last_update, show_pause_menu);
    if (show_exit_menu) {
        promptSaveOnExit(game);
    }
    textRenderer->RenderText("Will you outsmart the streets and claim your freedom?", 200.0f, window_height_px/5, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
    glfwSwapBuffers(game->get_window());
}

Entity StartState::createRotatingEarth() {
    Entity rotatingEarth = registry.create_entity();

    Motion &motion = registry.motions.emplace(rotatingEarth);
    motion.position = vec2(window_width_px / 2, window_height_px / 2);
    motion.scale = vec2(300, 300);

    RenderRequest renderRequest;
    renderRequest.used_texture = TEXTURE_ASSET_ID::ROTATING_EARTH;
    renderRequest.used_effect = EFFECT_ASSET_ID::TEXTURED;
    renderRequest.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    renderRequest.hasAnimation = true;
    renderRequest.animation.frameCount = 50;
    renderRequest.animation.currentFrame = 0;
    renderRequest.animation.frameTime = 60.f; // Adjust as needed to slow down/speed up the rotation
    renderRequest.animation.elapsedTime = 0.0f;
    renderRequest.animation.columns = 50;
    renderRequest.animation.rows = 1;
    renderRequest.animation.startRow = 1;
    renderRequest.animation.startCol = 1;
    registry.renderRequests.insert(rotatingEarth, renderRequest);
    return rotatingEarth;
}