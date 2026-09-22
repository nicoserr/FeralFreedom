#include "pause_state.hpp"
#include "world/world_system.hpp"
#include <iostream>
#include "core/ecs_registry.hpp"
#include "play_state.hpp"
#include "serialization/registry_serializer.hpp"

PauseState* PauseState::instance() {
    static PauseState instance;
    return &instance;
}

void PauseState::init(WorldSystem* game) {
    game->get_current_state()->show_pause_menu = true;
}

void PauseState::pause() {

}

void PauseState::resume() {

}

void PauseState::cleanup(WorldSystem* game) {
    GLuint shaderProgram = game->get_renderer()->getEffectProgram(EFFECT_ASSET_ID::BLACK);
    glUseProgram(shaderProgram);
    GLint is_paused_loc = glGetUniformLocation(shaderProgram, "is_paused");
    glUniform1i(is_paused_loc, GL_FALSE);
    GLint boxColor_loc = glGetUniformLocation(shaderProgram, "boxColor");
    glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); // remove box
    glUniform4f(boxColor_loc, color.r, color.g, color.b, color.a);

    game->get_current_state()->show_pause_menu = false;
}

void PauseState::reset(WorldSystem *game) {

}

void PauseState::handle_key_input(WorldSystem *game, int key, int scancode, int action, int mods) {
    if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE) {
        show_exit_menu = true;
    }

    if (show_exit_menu) {
        if (action == GLFW_RELEASE && key == GLFW_KEY_Y) {
            RegistrySerializer::saveRegistryState("PlayState", game); // Save game state
            glfwSetWindowShouldClose(game->get_window(), GLFW_TRUE);
        }
        if (action == GLFW_RELEASE && key == GLFW_KEY_N) {
            RegistrySerializer::cleanupStateFile("PlayState"); // Delete save file
            glfwSetWindowShouldClose(game->get_window(), GLFW_TRUE); // Exit without saving
        }
        if (action == GLFW_RELEASE && key == GLFW_KEY_C) {
            show_exit_menu = false;
        }
        return; // Skip other input processing while dialog is open
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        game->pop_state(); // Return to PlayState
    }
}

void PauseState::handle_mouse_input(WorldSystem* game, double xpos, double ypos) {
    // Check if the mouse click is inside any button
    for (const auto& button : buttons) {
        if (is_click_inside_button({xpos,ypos}, button, *game)) {

            return;
        }
    }
}

void PauseState::update(WorldSystem* game, float elapsed_ms) {
    // Update start screen logic (e.g., animations)
}

void PauseState::draw(WorldSystem* game, float elapsed_ms_since_last_update) {
    if (!show_pause_menu) return;

    if (show_exit_menu) {
        promptSaveOnExit(game);
    } else {
        const vec4 color = { 0.0f, 0.0f, 0.0f, 0.5f }; // Semi-transparent black
        float pause_box_width = window_width_px * 0.5f;
        float pause_box_height = window_height_px * 0.3f;
        float center_x = window_width_px / 2.0f;
        float center_y = window_height_px / 2.0f;

        // Draw the overlay box for pause menu
        game->get_renderer()->drawOverlayBox(center_x, center_y, pause_box_width, pause_box_height, color);
        game->get_renderer()->draw(elapsed_ms_since_last_update, true);
        // Draw centered pause text
        game->get_text_renderer()->RenderCenteredText("PAUSED", window_width_px, center_y + 20, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
        game->get_text_renderer()->RenderCenteredText("Press P to Resume", window_width_px, center_y - 10, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
        game->get_text_renderer()->RenderCenteredText("Press ESC to Quit", window_width_px, center_y - 40, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
    }

    glDisable(GL_BLEND);
    glfwSwapBuffers(game->get_window());
}



