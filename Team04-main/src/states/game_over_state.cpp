#include "game_over_state.hpp"
#include "world/world_system.hpp"
#include <iostream>
#include "core/ecs_registry.hpp"
#include "play_state.hpp"

GameOverState* GameOverState::instance() {
    static GameOverState instance;
    return &instance;
}

void GameOverState::init(WorldSystem* game) {

}

void GameOverState::pause() {

}

void GameOverState::resume() {

}

void GameOverState::cleanup(WorldSystem* game) {
    GLuint shaderProgram = game->get_renderer()->getEffectProgram(EFFECT_ASSET_ID::BLACK);
    glUseProgram(shaderProgram);
    GLint is_game_over_loc = glGetUniformLocation(shaderProgram, "is_game_over");
    glUniform1i(is_game_over_loc, GL_FALSE);
}

void GameOverState::reset(WorldSystem *game) {

}

void GameOverState::handle_key_input(WorldSystem *game, int key, int scancode, int action, int mods) {
    if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(game->get_window(), GLFW_TRUE); // Exit without saving
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        game->pop_state();
        game->restart_game();
    }
}

void GameOverState::handle_mouse_input(WorldSystem* game, double xpos, double ypos) {
    // Check if the mouse click is inside any button
    // for (const auto& button : buttons) {
    //     if (is_click_inside_button({xpos,ypos}, button, *game)) {
    //         start_again(game);
    //         return;
    //     }
    // }
}

void GameOverState::handle_mouse_movement(WorldSystem* game, double xpos, double ypos) {

}

void GameOverState::update(WorldSystem* game, float elapsed_ms) {
    // Update start screen logic (e.g., animations)
}

void GameOverState::draw(WorldSystem* game, float elapsed_ms_since_last_update) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // clean to black screen
    game->get_renderer()->draw(elapsed_ms_since_last_update, true);

    if (show_exit_menu) {
        promptSaveOnExit(game);
    } else {
        float center_y = window_height_px / 2.0f;
        GLuint shaderProgram = game->get_renderer()->getEffectProgram(EFFECT_ASSET_ID::BLACK);
        // set the game over flag
        glUseProgram(shaderProgram);
        GLint is_game_over_loc = glGetUniformLocation(shaderProgram, "is_game_over");
        glUniform1i(is_game_over_loc, GL_TRUE);
        // Draw centered game over text
        game->get_text_renderer()->RenderCenteredText("GAME OVER", window_width_px, center_y + 20, 1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        game->get_text_renderer()->RenderCenteredText("Press SPACE to Try Again", window_width_px, center_y - 20, 0.6f, glm::vec3(1.0f, 1.0f, 1.0f));
        game->get_text_renderer()->RenderCenteredText("Press ESC to Quit", window_width_px, center_y - 50, 0.6f, glm::vec3(1.0f, 1.0f, 1.0f));
    }
    glfwSwapBuffers(game->get_window());
}
