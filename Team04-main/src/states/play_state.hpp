#pragma once
#include <random>
#include <queue>
#include "game_state.hpp"
#include "systems/levels_rooms_system.hpp"
#include "systems/camera_system.hpp"
#include "systems/particle_system.hpp"

class PlayState : public GameState {
public:

    static PlayState* instance();

    void init(WorldSystem* game) override;
    void pause() override;
    void resume() override;
    void cleanup(WorldSystem* game) override;
    void reset(WorldSystem *game) override;

    void handle_key_input(WorldSystem *game, int key, int scancode, int action, int mods) override;
    void handle_mouse_input(WorldSystem *game, double xpos, double ypos) override;
    void handle_mouse_movement(WorldSystem *game, double xpos, double ypos) override {}; 
    void update(WorldSystem* game, float elapsed_ms) override;
    void draw(WorldSystem* game, float elapsed_ms_since_last_update) override;
    RenderSystem* renderer = nullptr;
    TextRenderer* textRenderer = nullptr;
    ShadowRenderer* shadowRenderer = nullptr;
    LevelSystem* level_manager;
    ParticleSystem* particleSystem = nullptr;
    ParticleSystem* sparkleSystem = nullptr;

    bool tutorialFinished;

private:
    PlayState() : rng(std::random_device()()), random_step(3, 10), random_angle(0, 7){}
    float PLAYER_SPEED = 300;
    const float CHASE_DURATION = 2000;
    const float CONVERGER_UPDATE_TIME = 3000;

    vec2 playerControls = {0,0};
    KeyInputState inputState;

    void updatePlayerAnimation();
    void updatePatrolAnimation(Entity patrol);
    void updateDogAnimation(Entity dog);

    void beginEncounter(WorldSystem* game, Entity player_entity, Entity npc_entity);

    // motion handling
    void resetPatrolMovement();
    void updateRandomWalkers();
    void updateChasers(float elapsed_ms) const;
    void updateConvergers(float elapsed_ms);
    void updatePatrolRendering(Motion &patrol_motion, Entity patrol);

    void change_direction(Motion &motion);

    void itemCollection(Entity player);
    void renderPickupText(WorldSystem* game);
    void renderConsumeText( WorldSystem* game);
    void renderKeyText(WorldSystem* game);
    void renderDialogueText(WorldSystem* game);
    void renderSelectBox(WorldSystem* game);
    void renderSelectArrow(WorldSystem* game);
    void checkInNPCRange(WorldSystem* game);

    void printStats(const Stats& playerStats);
    void updateStats(Stats& playerStats, Stats& itemStats, bool isAdding);
    void createKey(vec2 position);
    void drawKey();
    void buryPlayer(Entity player, Room* currentRoom);

    Entity keyIconEntity;

    bool chase_active = false;
    bool chase_disengaged = false;
    bool ateBadFood;
    bool droppedItem;
    float textTimerS = 3;
    float fpsCount = 0;
    float fpsTimer = 0;

    vec4 overlay_color = {0,0,0,0};
    float light_amount = 0.0;
    Entity day_night_entity;

    CameraSystem cameraSystem;
    
    bool shouldRenderEPress = false;
    Entity e_press = registry.create_entity();
    Animation e_press_animation;
    RenderRequest e_press_request;

    Entity dialogueNPC;
    std::string currentMessage = "";
    std::queue<std::string> currentDialogue;
    Entity dialogueBox = registry.create_entity();

    Entity dialogueEPress = registry.create_entity();
    Animation dialogue_e_animation;
    RenderRequest dialogue_e_request;

    Entity selectBox = registry.create_entity();
    Entity selectArrow = registry.create_entity();
    int selectArrowPos = 1;

    bool shouldShowDialogue = false;
    bool shouldPickUpItem = false;

    bool shouldShowSelect = false;

    // C++ random number generator
    std::default_random_engine rng;
    std::uniform_real_distribution<float> random_step;
    std::uniform_int_distribution<int> random_angle;
    std::vector<float> angles = {0, M_PI / 4, M_PI / 2, 3 * M_PI / 4, M_PI, 5 * M_PI / 4, 3 * M_PI / 2, 7 * M_PI / 4};
};
