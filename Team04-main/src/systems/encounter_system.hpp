#pragma once

#include "core/ecs_registry.hpp"
#include "core/ecs.hpp"
#include "../common.hpp"
#include "render_system.hpp"
#include "systems/text_renderer.hpp"
#include <vector>
#include <string>
#include <random>
#include <queue>

enum class ACTION_TYPE {
    ATTACK,
    CHARM,
    INTIMIDATE,
    TYPES_SIZE
};

enum class ENCOUNTER_STATUS {
    DEFAULT,
    WAITING_FOR_PLAYER_ACTION,
    NPC_ACTION,
    PLAYER_ACTION,
    LOST,
    W_KNOCKOUT,
    W_CHARMED,
    W_INTIMIDATED,
    TYPES_SIZE, 
    ATTACKED,
    DIALOGUE
};

enum class PLAYER_ACTION_RESULT {
    NONE,
    IN_PROGRESS,
    HIT,
    MISS
};

class EncounterSystem {
    public:
        EncounterSystem();
        ~EncounterSystem();

        /*run this function at the start of an encounter, it will load the stats from player and npc
        into the encounter system for some data that needs to be calculated on the fly
        NOTE: make sure that the player and the npc have their components in the registry*/
        void initEncounter(Entity player, Entity npc);

        // returns the outcome of the action as a string, eg. "[player name] hit [npc name] for (1d6(4) + 3 = 7) points of damage"
        void doPlayerAction(ACTION_TYPE type);

        void doNPCAction(ACTION_TYPE type);

        std::queue<std::string> encounterMessages;
        int npc_last_health;
        float npc_last_emotion;

        bool isEncounterOver(std::string& message, Stats player_stats, Stats npc_stats);

        void setEncounterOverStatus(Stats player_stats, Stats npc_stats);

        void addEncounterOverMsg(Stats player_stats, Stats npc_stats);

        //return whether the enocunter is ongoing, lost, or won, and if won, what type of victory
        ENCOUNTER_STATUS getStatus();

        // update status
        void setStatus(ENCOUNTER_STATUS curr_status);

        /*returns the current emotion (progress to getting charmed or intimidated) of the npc
        as a fraction (from range from -1 to 1), where -1 means the npc is fully intimided and +1
        means the npc is fully charmed, and 0 means the npc haven't been affected at all
        (note that the charm threshold does not necessarily equal the intimidate threshold,
        this needs to be calculated as a normalized value)*/
        float getNormalizedEmotion();

        //reset the internal states of the encounter
        void resetEncounter();

        //function are testing if the rng works as intended, remove later
        //rolls a d6 if true, rolls a d20 if false
        int rollADice(bool which);

        void createEncounter(int npc_texture_id, std::string npc_name, Entity npc, Stats npc_stats, 
        Entity npc_health_bar,Entity npc_emotion_bar, Entity emotion_bar_ticker, 
        Entity encounter_text_box, Entity encounter_attack_box, RenderSystem* renderer, TextRenderer* textRenderer, Entity cat_attack);

        void renderNPC(int npc_texture_id, Entity npc, RenderSystem* renderer);

        void renderNPCName(TextRenderer* textRenderer, std::string npc_name, Entity npc);
        
        void renderNPCHealthBar(Stats npc_stats, Entity npc, Entity npc_health_bar);

        void renderNPCHealthText(TextRenderer* textRenderer, Stats npc_stats, Entity npc_health_bar);

        void renderNPCEmotionBar(Stats npc_stats, Entity npc, Entity npc_emotion_bar, Entity emotion_bar_ticker);

        void renderNPCEmotionText(TextRenderer* textRenderer, Stats npc_stats, Entity npc_emotion_bar);

        void renderEncounterText(TextRenderer* textRenderer, std::string& message, Entity text_box);

        void renderPlayerHealth(TextRenderer* textRenderer, int currentHp, vec2 position);

        void renderEncounterTextBox(Entity encounter_text_box);

        void renderAttackBox(Entity encounter_attack_box);

        std::string encounterStatusToString(ENCOUNTER_STATUS status);

        void updateStatsVisuals(Stats npc_stats, Entity npc_emotion_bar, Entity emotion_bar_ticker, Entity npc_health_bar);

        void renderActionButtonText(TextRenderer* textRenderer);

        void renderPlayerStats(TextRenderer* textRenderer, Stats& playerStats);

        void renderCatAttack(Entity cat_attack);

        void successfulAttack(Entity player, Entity npc, bool successful);

        float calculateProjectileSpawnInterval(Stats npcStat);

        void createProjectile();

        void updateNPCAction(float elapsed_ms);

        void updateProjectiles(float elapsed_ms);

        void updateWaveWalls(float elapsed_ms);

        void checkEncounterCollision(Entity cat_attack);

        float calculateWaveSpawnInterval(Stats npcStat);

        float calculateWaveVelocity(Stats npcStat);

        void createWaveWall();

        void updateBounceProjectiles(float elapsed_ms);

        void createBounceProjectile();

        bool attacking = false;
        
        PLAYER_ACTION_RESULT playerResult;
        bool npcAttacking = false;
        int npcAttackPattern = -1;

        float grid_row = window_height_px/6.0f;

        float attack_box_x = 0;
        float attack_box_y = 0;
        float attack_box_width = 0;

        float timeSinceLastSpawn = 0.0f;
        float timeSinceAttackStarted = 0.0f;
        float spawnInterval = 1.0f;

        std::vector<Entity> activeColliders;

        Mix_Chunk* encounter_music;
        Mix_Chunk* p_charm_sound;
        Mix_Chunk* p_attack_sound;
        Mix_Chunk* p_intimidate_sound;
        Mix_Chunk* npc_attack_sound;
        Mix_Chunk* p_hurt_sound;

        std::default_random_engine rng;

        bool encounterOver = true;

        bool tutorialEncounterAttackComplete = false;
        bool tutorialNPCAttackComplete = false;
        bool tutorialCharmIntimidateComplete = false;
        bool tutorialComplete = false;
    private:
        const float BUTTON_Y_POS = window_height_px/8;
        const float BUTTON_TEXT_Y_POS = BUTTON_Y_POS + 4*1280.0f * window_width_px*0.8f/1440.0f * 0.5/20.f;

        const float damageCooldownDuration = 1.5f;
        float damageCooldown = 0.0f;
        float damageFlickerInterval = 0.05f;
        float damageFlickerTimer = 0.0f;

        float waveVelocity = 0.0f;

        int maxBounceProjectiles = 6;

        ENCOUNTER_STATUS status;
        
        Entity currentPlayer;
        Entity currentNpc;
        int npcEmotion;
        // TODO: update dynamically when ready.
        std::string playerName = "[PLAYER]";
        std::string npcName = "[NPC]";

        std::uniform_int_distribution<int> d6;
        std::uniform_int_distribution<int> d20;
};

extern EncounterSystem encounter;