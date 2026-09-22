#include "world/world_system.hpp"
#include <fstream>

#include "states/game_over_state.hpp"
#include "states/play_state.hpp"
#include "../common.hpp"
#include "world/world_init.hpp"
#include "core/ecs_registry.hpp"
#include "states/start_state.hpp"
#include "serialization/registry_serializer.hpp"

// Game Configuration
float fpsTimer = 0.0f;
float fpsCount = 0;

WorldSystem::WorldSystem() {
}

WorldSystem::~WorldSystem() {
    // To Destroy Music Components
    // if (background_music != nullptr)
    //     Mix_FreeMusic(background_music);
    /*
    if (salmon_dead_sound != nullptr)
        Mix_FreeChunk(salmon_dead_sound);
    if (salmon_eat_sound != nullptr)
        Mix_FreeChunk(salmon_eat_sound);
        */

    // To close Audio
    // Mix_CloseAudio();
}

// Debugging
namespace {
	void glfw_err_cb(int error, const char *desc) {
		fprintf(stderr, "%d: %s", error, desc);
	}
}

GLFWwindow* WorldSystem::create_window() {
    glfwSetErrorCallback(glfw_err_cb);
    if(!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW");
        return nullptr;
    }

    //-------------------------------------------------------------------------
	// If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
	// enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
	// GLFW / OGL Initialization
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    getWindowSize();
    window = glfwCreateWindow(window_width_px, window_height_px, "Feral Freedom", nullptr, nullptr);
    if(window == nullptr) {
        fprintf(stderr, "Failed to glfwCreateWindow");
        return nullptr;
    }

    glfwMaximizeWindow(window);
    //Set callbacks to member functions 
    glfwSetWindowUserPointer(window, this);
    auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
    auto mouse_button_redirect = [](GLFWwindow* wnd, int button, int action, int mods) {
        double xpos, ypos;
        glfwGetCursorPos(wnd, &xpos, &ypos);
        if (action == GLFW_PRESS) {
            ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_click(xpos, ypos);
        }
    };
    auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
    glfwSetKeyCallback(window, key_redirect);
    glfwSetMouseButtonCallback(window, mouse_button_redirect);
    glfwSetCursorPosCallback(window, cursor_pos_redirect);

    //////////////////////////////////////
    // Loading music and sounds with SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Failed to initialize SDL Audio");
        return nullptr;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
        fprintf(stderr, "Failed to open audio device");
        return nullptr;
    }
    return window;
}

void WorldSystem::init(RenderSystem* renderer_arg, LevelSystem* level_manager_arg, CollisionSystem* collision_manager_arg, TextRenderer* text_renderer_arg, ShadowRenderer* shadow_renderer, PathFindingSystem* path_finding_system_arg) {
    this->renderer = renderer_arg;
    this->levelManager = level_manager_arg;
    this->collisionManager = collision_manager_arg;
    this->text_renderer = text_renderer_arg;
    this->path_finding_system = path_finding_system_arg;
    this->shadow_renderer = shadow_renderer;

    // printf("Initializing world\n");
    change_state(StartState::instance());
}

// Should the game be over ?
bool WorldSystem::is_over() const {
	return bool(glfwWindowShouldClose(window));
}

bool WorldSystem::step(float elapsed_ms_since_last_update) {
    // update to the current state and call that state's functions to handle actions
    if (!get_current_state()) {
        return false;
    }

    if (game_over) {
        get_current_state()->renderHearts(0);  // to show no hearts left
        push_state(GameOverState::instance());
        game_over = false;
    }

    for (Entity entity: registry.deathTimers.entities) {
        get_current_state()->buryPlayer(entity, levelManager->currentLevel->currentRoom);
        registry.deathTimers.remove(entity);
        // levelManager->reloadCurrentLevel(entity);
        return true;
    }

    // clear the screen with the background color of the current state
    get_current_state()->draw(this, elapsed_ms_since_last_update);
    // update the state logic (after drawing)
    get_current_state()->update(this, elapsed_ms_since_last_update);
    return true;
}

//on key callback
void WorldSystem::on_key(int key, int scancode, int action, int mod) {
    if (get_current_state()) {
        // Pass the mouse click event to the current state's mouse handler
        get_current_state()->handle_key_input(this, key, scancode, action, mod);
    }
}

void WorldSystem::on_mouse_move(vec2 mouse_position) {
    (vec2)mouse_position;
    get_current_state()->handle_mouse_movement(this, mouse_position.x, mouse_position.y);
}


//NOTE: Game state becomes start only when mouse clicks within coordinates set below.
//So far with no actual buttons just have to click on that area of the screen.
void WorldSystem::on_mouse_click(double xpos, double ypos) {
    if (get_current_state()) {
        // Pass the mouse click event to the current state's mouse handler
        get_current_state()->handle_mouse_input(this, xpos, ypos);
    }
}

// SCREEN STATE MANAGEMENT
void WorldSystem::change_state(GameState* state) {
    if (!states.empty() && states.back() != state) {
        states.back()->cleanup(this);
        states.pop_back();
    }
    states.push_back(state);
    states.back()->init(this);
}

void WorldSystem::push_state(GameState* state) {
    if (!states.empty()) {
        states.back()->pause();
    }
    states.push_back(state);
    states.back()->init(this);
}

void WorldSystem::pop_state() {
    if (!states.empty()) {
        states.back()->cleanup(this);
        states.pop_back();
    }
    if (!states.empty()) {
        states.back()->resume();
    }
}

GameState* WorldSystem::get_current_state() const {
    if (states.empty()) return nullptr;
    return states.back();
}

// not needed
void WorldSystem::cleanup() {
    // Clean up the game
    // printf("Closing the game.\n");

    // Deletes files for each state and reset level system. Modify if we want save-load within game.
    for (GameState* state : states) {
        state->reset(this);
    }


    // cleanup_load_save_directory();

    /*
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
    */
}

void WorldSystem::restart_game() {
    registry.get_entities().clear();
    registry.clear_all_components();


    // Clean restart. Modify if needed. Can alternatively use cleanup_load_save_directory();
    for (GameState* state : states) {
        state->reset(this);
    }
    // RegistrySerializer::saveRegistryState("PlayState", this);
    change_state(PlayState::instance());
}
