#pragma once
#include "game_state.hpp"
#include "../systems/encounter_system.hpp"
#include <string>

class EncounterState : public GameState {
public:

    static EncounterState* instance(Stats player, Stats npc, int p_id, int texture_id, std::string name, bool isTutorialNPC);

    void init(WorldSystem* game) override;

    void cleanup(WorldSystem* game) override;
    void pause() override;
    void resume() override;
    void reset(WorldSystem *game) override;

    void handle_key_input(WorldSystem *game, int key, int scancode, int action, int mods) override;
    void handle_mouse_input(WorldSystem* game, double xpos, double ypos) override; // implement for clicking on objects
    void handle_mouse_movement(WorldSystem* game, double xpos, double ypos) override;
    void update(WorldSystem* game, float elapsed_ms) override;
    void draw(WorldSystem* game, float elapsed_ms_since_last_update) override;

    bool isGestureMatch(const Gesture& targetGesture);
    void addPoint(vec2 newPoint);
    void renderMisc();
    void renderTarget();
    void renderPath();
    void chooseTarget();
    vec2 cubicBezierInterpolation(float t);
    void updateCurve(float elapsed_ms);
    vec2 quadraticBezierInterpolation(const vec2& p0, const vec2& p1, const vec2& p2,float t);

    float playerAttackStartTimer = 0.0f;

    static Gesture line;
    static Gesture triangle;
    static Gesture zigzag;
    static Gesture currentTarget;
    static Gesture bezierTarget;

    std::string currentMessage;

private:
    EncounterState() = default;

    const float MS_PER_TURN = 1750.f;
    const float CUTENESS_WIN_WEIGHT = 0.15f;
    const float REPUTATION_WIN_WEIGHT = 0.1f;
    const float KNOCKOUT_WIN_FEROCITY_WEIGHT = 0.1;
    const float KNOCKOUT_WIN_REPUTATION_WEIGHT = 0.04;
    
    std::vector<glm::vec2> currentGesturePath;

    static Entity player_entity;
    static Entity npc_entity;
    static Entity npc_health_bar;
    static Entity npc_emotion_bar;
    static Entity npc_emotion_bar_ticker;
    static Entity encounter_text_box;
    static Entity encounter_attack_box;
    static Entity cat_attack;
    static Stats player_stats;
    static Stats npc_stats;
    static Entity attack_button;
    static int current_player_hp;
    static int player_index;
    static int npc_texture_id;
    static std::string encounter_message;
    static std::string npc_name;
    static EncounterSystem encounter_system;
    static float ms_since_status_update;
    static bool isTutorial;

    bool displayEndScreen = false;
    bool displayBeginningScreen = true;
    std::string endScreenMessage;
    static bool wasMouseClicked;
    static float currentT;
    static float step;

    Entity e_press = registry.create_entity();
    Animation e_press_animation;
    RenderRequest e_press_request;

    std::default_random_engine rng;

    void handleKnockoutWin(WorldSystem * game);
    void handleCutenessWin(WorldSystem * game);
    void handleReputationWin(WorldSystem * game);
    void handleLoss(WorldSystem * game);
};