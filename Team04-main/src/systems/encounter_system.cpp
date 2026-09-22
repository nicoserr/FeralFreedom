#pragma once

#include "encounter_system.hpp"

EncounterSystem encounter;

EncounterSystem::EncounterSystem() {
    rng = std::default_random_engine(std::random_device()());
    d6 = std::uniform_int_distribution<int>{1,6};
    d20 = std::uniform_int_distribution<int>{1,20};
}

EncounterSystem::~EncounterSystem() {

}

void EncounterSystem::initEncounter(Entity player, Entity npc)
{   
    NPC& npc_npc = registry.npcs.get(npc);
    if(npc_npc.name == "Raccoon") {
        // for(int i = 0; i < 6; i++) {
            //encounterMessages.push(npc_npc.dialogue[i]);
            //std::cout << npc_npc.dialogue[i] << std::endl;
        // }

        // Manual adding rn since I can't seem to get the dialogue var to work correctly
        encounterMessages.push("Raccoon: Before you go into the open world you're going to have to learn how to deal with encounters");
        encounterMessages.push("Raccoon: On your turn, you will have 3 options, Attack, Charm, and Intimidate");
        encounterMessages.push("Raccoon: Below me, you will be able to see my health bar and also my current emotions");
        encounterMessages.push("Raccoon: To successfully defeat an encounter, you have to bring my health to 0, or move my emotion bar all the way to the left or right");
        encounterMessages.push("Raccoon: First, let's try the Attack action");
        encounterMessages.push("Raccoon: Using your mouse, you'll need to click and hold and trace the shape that is being displayed to the right. Click on the Attack button and give it a try!");
        status = ENCOUNTER_STATUS::DIALOGUE;
    } else {
        status = ENCOUNTER_STATUS::WAITING_FOR_PLAYER_ACTION;
    }
    currentPlayer = player;
    currentNpc = npc;
    // npcEmotion is + if moving towards cuteness (charm), and - if moving towards reputation (intimidate)
    npcEmotion = 0.0f;

    Stats& npc_stats = registry.stats.get(npc);

    maxBounceProjectiles = 6;
    
    int extraBalls = npc_stats.ferocity/15;
    if(extraBalls > 6) {
        extraBalls = 6;
    }
    maxBounceProjectiles += extraBalls; // Base 6 projectiles, add 1 for every 3 npc ferocity capped at 6

    assert((
    //     !registry.players.has(currentPlayer)
    // || !registry.npcs.has(currentNpc)
    // ||
    registry.stats.has(currentPlayer) && registry.stats.has(currentNpc))
     && "Invalid Entities for Starting Encounter, Required Components not Found");
}

void EncounterSystem::doPlayerAction(ACTION_TYPE type)
{
    status = ENCOUNTER_STATUS::PLAYER_ACTION;
    playerResult = PLAYER_ACTION_RESULT::IN_PROGRESS;
    std::string message = "";

    Stats& playerStats = registry.stats.get(currentPlayer);
    Stats& npcStats = registry.stats.get(currentNpc);

    switch (type) {
        case ACTION_TYPE::ATTACK: {
            
            attacking = !attacking;
            break;
        }
        case ACTION_TYPE::CHARM: {
            int charmRoll = d6(rng);
            npcEmotion += charmRoll + playerStats.cuteness;
            if(npcEmotion > npcStats.cuteness) {
                npcEmotion = npcStats.cuteness;
            };

            encounterMessages.push(playerName + " tries to charm " + npcName + ".\n");
            encounterMessages.push(npcName + " likes " + playerName + " more!\n");
            if(!tutorialCharmIntimidateComplete && registry.npcs.get(currentNpc).isTutorialNPC) {
                encounterMessages.push("Raccoon: When you charm or intimidate, it calculates a dice roll value based on your reputation or charm stats");
                encounterMessages.push("Raccoon: That's all there is to encounters!");
                encounterMessages.push("Raccoon: Complete the rest of this encounter on your own and get out there into the big world!");
                tutorialCharmIntimidateComplete = true;
            }
            setStatus(ENCOUNTER_STATUS::ATTACKED);
            break;
        }
        case ACTION_TYPE::INTIMIDATE: {
            int intimidateRoll = d6(rng);
            npcEmotion -= intimidateRoll + playerStats.reputation;
            if(npcEmotion < -npcStats.reputation) {
                npcEmotion = -npcStats.reputation;
            }

            encounterMessages.push(playerName + " tries to intimidate " + npcName + ".\n");
            encounterMessages.push(npcName + " fears " + playerName + " more!\n");
            if(!tutorialCharmIntimidateComplete && registry.npcs.get(currentNpc).isTutorialNPC) {
                encounterMessages.push("Raccoon: When you charm or intimidate, it calculates a dice roll value based on your reputation or charm stats");
                encounterMessages.push("Raccoon: That's all there is to encounters!");
                encounterMessages.push("Raccoon: Complete the rest of this encounter on your own and get out there into the big world!");
                tutorialCharmIntimidateComplete = true;
            }
            setStatus(ENCOUNTER_STATUS::ATTACKED);
            break;
        }
        default:
        break;
    }
}

void EncounterSystem::successfulAttack(Entity player, Entity npc, bool successful){
    Stats& playerStats = registry.stats.get(player);
    Stats& npcStats = registry.stats.get(npc);
    if (successful) {
        playerResult = PLAYER_ACTION_RESULT::HIT;
        // this is a hit
        encounterMessages.push(playerName + " hits " + npcName + " with an attack!\n");
        //     [1d20 = "
        // + std::to_string(playerRoll) + " + " + std::to_string(playerStats.agility) + " vs 1d20 = "
        // + std::to_string(npcRoll) + " + " + std::to_string(npcStats.agility) + "]\n");

        int damageRoll = d6(rng);
        npcStats.currentHp -= damageRoll + std::floor(playerStats.ferocity);
        if(npcStats.currentHp < 0) {
            npcStats.currentHp = 0;
        }
        
        npcEmotion -= playerStats.ferocity/3 + 1;

        encounterMessages.push(npcName + " took [" + std::to_string(damageRoll + static_cast<int>(std::floor(playerStats.ferocity)))
            +"] points of damage!\n");
            encounterMessages.push(npcName +  " fears " + playerName + " more after being attacked!\n");

        if(!tutorialEncounterAttackComplete && registry.npcs.get(npc).isTutorialNPC) {
            encounterMessages.push("Raccoon: Nice, you traced it well and dealt some damage");
            encounterMessages.push("Raccoon: When you damage an enemy, they will also be slightly intimidated");
            encounterMessages.push("Raccoon: The shape that you will be asked to trace is randomly selected");
            encounterMessages.push("Raccoon: After you attack, it's the enemy's turn to attack");
            encounterMessages.push("Raccoon: Use your mouse and try to avoid colliding with anything in the box!");
        }
    } else {
        playerResult = PLAYER_ACTION_RESULT::MISS;
        //this is a miss
        encounterMessages.push(playerName + " misses " + npcName + " with an attack!");

        if(!tutorialEncounterAttackComplete && registry.npcs.get(npc).isTutorialNPC) {
            encounterMessages.push("Raccoon: Uh oh, you didn't trace well enough and missed your attack");
            encounterMessages.push("Raccoon: The shape that you will be asked to trace is randomly selected");
            encounterMessages.push("Raccoon: After you attack, it's the enemy's turn to attack");
            encounterMessages.push("Raccoon: Use your mouse and try to avoid colliding with anything in the box!");
        }
    }
}

void EncounterSystem::doNPCAction(ACTION_TYPE type)
{
    status = ENCOUNTER_STATUS::NPC_ACTION;
    std::string message = "";

    Stats& playerStats = registry.stats.get(currentPlayer);
    Stats& npcStats = registry.stats.get(currentNpc);

    switch (type) {
        case ACTION_TYPE::ATTACK: {
            npcAttacking = !npcAttacking;
            timeSinceLastSpawn = 0.0f;
            timeSinceAttackStarted = 0.0f;
            if(npcAttackPattern == 1) {
                spawnInterval = calculateProjectileSpawnInterval(npcStats);
            } else if(npcAttackPattern == 2) {
                spawnInterval = calculateWaveSpawnInterval(npcStats);
                waveVelocity = calculateWaveVelocity(npcStats);
            } else if(npcAttackPattern == 3) {
                spawnInterval = calculateProjectileSpawnInterval(npcStats);
            }
            break;
        }
        default:
        break;
    }
}

bool EncounterSystem::isEncounterOver(std::string &message, Stats player_stats, Stats npc_stats) {
    
    if (player_stats.currentHp <= 0) {
        return true;
    } else if (npc_stats.currentHp <= 0) {
        return true;
    } else if (npcEmotion <= npc_stats.reputation * -1) {
        return true;
    } else if (npcEmotion >= npc_stats.cuteness) {
        return true;
    } else {
        return false;
    }
}

void EncounterSystem::setEncounterOverStatus(Stats player_stats, Stats npc_stats) {
    if (player_stats.currentHp <= 0) {
        // Player lost.
        status = ENCOUNTER_STATUS::LOST;
    } else if (npc_stats.currentHp <= 0) {
        // Player won! Knocked out NPC.
        status = ENCOUNTER_STATUS::W_KNOCKOUT;
    } else if (npcEmotion <= npc_stats.reputation * -1) {
        // Player won! Intimidated NPC.
        status = ENCOUNTER_STATUS::W_INTIMIDATED;
    } else if (npcEmotion >= npc_stats.cuteness) {
        // Player won! Charmed NPC.
        status = ENCOUNTER_STATUS::W_CHARMED;
    }
}

void EncounterSystem::addEncounterOverMsg(Stats player_stats, Stats npc_stats) {
    if (player_stats.currentHp <= 0) {
        // Player lost.
        encounterMessages.push(playerName + " is knocked out!\n");
    } else if (npc_stats.currentHp <= 0) {
        // Player won! Knocked out NPC.
        encounterMessages.push(playerName + " knocks " + npcName + " out!\n");
    } else if (npcEmotion <= npc_stats.reputation * -1) {
        // Player won! Intimidated NPC.
        encounterMessages.push(npcName + " is scared away by " + playerName + "!\n");
    } else if (npcEmotion >= npc_stats.cuteness) {
        // Player won! Charmed NPC.
        encounterMessages.push(npcName + " is won over by " + playerName + "'s cuteness!\n");
    }
}

ENCOUNTER_STATUS EncounterSystem::getStatus()
{
    return status;
}

void EncounterSystem::setStatus(ENCOUNTER_STATUS curr_status)
{
    status = curr_status;
}

float EncounterSystem::getNormalizedEmotion()
{
    if(npcEmotion == 0) {
        return 0.0f;
    } else {
        Stats& npcStats = registry.stats.get(currentNpc);
        if (npcEmotion > 0) {
            float charmThreshold = npcStats.cuteness;
            float normalized = (float)npcEmotion/charmThreshold;
            if (normalized > 1.0f)
                normalized = 1.0f;
            return normalized;
        } else {
            float intimidateThreshold = npcStats.reputation;
            float normalized = (float)npcEmotion/intimidateThreshold;
            if (normalized > 1.0f)
                normalized = 1.0f;
            return normalized;
        }
    }
}

void EncounterSystem::resetEncounter()
{
    status = ENCOUNTER_STATUS::DEFAULT;
    npcEmotion = 0.0f;
}

int EncounterSystem::rollADice(bool which)
{
    if (which) {
        return d6(rng);
    } else {
        return d20(rng);
    }
}

void EncounterSystem::createEncounter(int npc_texture_id, std::string npc_name, Entity npc, Stats npc_stats, 
Entity npc_health_bar, Entity npc_emotion_bar, Entity emotion_bar_ticker, Entity encounter_text_box, 
Entity encounter_attack_box, RenderSystem* renderer, TextRenderer* textRenderer, Entity cat_attack) {
    // create necessary renders for the encounter
    renderNPC(npc_texture_id, npc, renderer);
    renderNPCHealthBar(npc_stats, npc, npc_health_bar);
    renderNPCEmotionBar(npc_stats, npc, npc_emotion_bar, emotion_bar_ticker);
    // Initialize the text box first and have a separte function for rendering cause we might
    // want it to disappear/reappear during different parts of the encounter
    // Might move this to another function called createTextBox() or something later

    float text_box_width_percentage = 0.8f;
    float text_box_width = window_width_px * text_box_width_percentage;

    float scale_factor = text_box_width/1440.0f;  // The text box texture is 1440x1280
    float box_width = 1440.0f * scale_factor * 0.7f;
    float box_height = 300.0f * scale_factor * 0.7f;

    float text_box_x = window_width_px / 2.0f;
    float text_box_y = window_height_px/6.0f * 3.5f;

    // Text box Motion
    Motion& text_box_m = registry.motions.emplace(encounter_text_box);
    text_box_m.position = {text_box_x, text_box_y};
    text_box_m.scale = {box_width, box_height};
    text_box_m.velocity = {0, 0};

    float attack_box_width_percentage = 0.3f;
    attack_box_width = window_width_px * attack_box_width_percentage;

    float attack_scale_factor = attack_box_width/350.0f;
    box_width = 350.0f * attack_scale_factor * 0.6f;
    box_height = 350.0f * attack_scale_factor * 0.6f;

    attack_box_width = box_width;
    attack_box_x = text_box_x;
    attack_box_y = text_box_y;

    Motion& attack_box_m = registry.motions.emplace(encounter_attack_box);
    attack_box_m.position = {text_box_x, text_box_y};
    attack_box_m.scale = {box_width, box_height};
    attack_box_m.velocity = {0, 0};
}

// RENDERING
void EncounterSystem::renderNPC(int npc_texture_id, Entity npc, RenderSystem* renderer) {
    float target_npc_height = 0.2f * window_height_px;

    float scale_y = target_npc_height / 96.0f; //Assuming that all npc textures will be 48x96
    float scale_x = scale_y;

    Motion& npc_motion = registry.motions.emplace(npc);
    npc_motion.position = {window_width_px/2.0f, window_height_px/6.0f};
    npc_motion.scale = {48.0f * scale_x, 96.0f * scale_y};
    npc_motion.velocity = {0, 0};

    TEXTURE_ASSET_ID npc_texture = static_cast<TEXTURE_ASSET_ID>(npc_texture_id);

    RenderRequest npc_render_request = {
        npc_texture,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    registry.renderRequests.insert(npc, npc_render_request);
}

void EncounterSystem::renderNPCName(TextRenderer* textRenderer, std::string name, Entity npc) {
    Motion& m = registry.motions.get(npc);
    float text_scale = m.scale.y / 96.0f / 4.0f; // Set so that text is 1/4 size of npc sprite

    GLfloat text_x = window_width_px/2.0f - (name.length() * 48 * text_scale) / 2.0f; // Center-align the text
    //GLfloat text_y = window_height_px - m.position.y + m.scale.y / 2.0f - (48 * text_scale);
    
    GLfloat text_y = window_height_px - (window_height_px/6.0f)/2.0f;

    textRenderer->RenderText(name, text_x, text_y, text_scale, glm::vec3(1.0f, 1.0f, 1.0f));
}

void EncounterSystem::renderNPCHealthBar(Stats npc_stats, Entity npc, Entity npc_health_bar) {
    Motion& m = registry.motions.get(npc);

    float max_health_bar_width_percent = 0.1f;
    float max_health_bar_width = window_width_px * max_health_bar_width_percent;

    float current_health_percent = static_cast<float>(npc_stats.currentHp)/ npc_stats.maxHp;
    float health_bar_width = max_health_bar_width * current_health_percent;

    npc_last_health = npc_stats.currentHp;

    float health_bar_x = m.position.x - (max_health_bar_width/4);
    //float health_bar_y = m.position.y + (m.scale.y / 2) + 40.0f;
    float health_bar_y = window_height_px/6.0f * 2.0f;

    Motion& health_bar_motion = registry.motions.emplace(npc_health_bar);
    health_bar_motion.position = {health_bar_x, health_bar_y};
    health_bar_motion.scale = {health_bar_width, m.scale.y/3.0f};
    health_bar_motion.velocity = {0, 0};

    RenderRequest npc_health_bar_request = {
        TEXTURE_ASSET_ID::HEALTH_BAR,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    registry.renderRequests.insert(npc_health_bar, npc_health_bar_request);
}

void EncounterSystem::renderNPCHealthText(TextRenderer* textRenderer, Stats npc_stats, Entity npc_health_bar) {
    float max_health_bar_width_percent = 0.1f;
    float max_health_bar_width = window_width_px * max_health_bar_width_percent;
    float health_bar_x = window_width_px/2.0f - (max_health_bar_width/4);

    float text_scale = (max_health_bar_width / 3.0f) / 200.0f;
    std::string health_text = std::to_string(npc_stats.currentHp) + " / " + std::to_string(npc_stats.maxHp);
    GLfloat text_x = health_bar_x + max_health_bar_width * 0.5f;
    GLfloat text_y = window_height_px - (window_height_px/6.0f)*2.03f;

    textRenderer->RenderText(health_text, text_x, text_y, text_scale, glm::vec3(1.0f, 1.0f, 1.0f));
}

void EncounterSystem::renderPlayerHealth(TextRenderer *textRenderer, int currentHP, vec2 position) {
    float text_scale = 0.5f;
    Stats& playerStats = registry.stats.get(currentPlayer);
    std::string player_health = "HP: " + std::to_string(currentHP) + " / " + std::to_string(playerStats.maxHp);
    GLfloat text_x = position.x;
    GLfloat text_y = position.y;

    textRenderer->RenderText(player_health, text_x, text_y, text_scale,glm::vec3(1.0f, 1.0f, 1.0f));
}

void EncounterSystem::renderNPCEmotionBar(Stats npc_stats, Entity npc, Entity npc_emotion_bar, Entity emotion_bar_ticker) {
    Motion& m = registry.motions.get(npc);
    npc_last_emotion = npcEmotion;

    float max_emotion_bar_width_percent = 0.15f;
    float max_emotion_bar_width = window_width_px * max_emotion_bar_width_percent;

    float emotion_bar_x = window_width_px/2.0f;
    float emotion_bar_y = window_height_px/6.0f * 2.2f;

    if(!registry.motions.has(npc_emotion_bar)){
        registry.motions.emplace(npc_emotion_bar);
    }
    Motion& emotion_bar_motion = registry.motions.get(npc_emotion_bar);
    emotion_bar_motion.position = {emotion_bar_x, emotion_bar_y};
    emotion_bar_motion.scale = {max_emotion_bar_width, m.scale.y/2.5f};
    emotion_bar_motion.velocity = {0, 0};

    RenderRequest npc_emotion_bar_request = {
        TEXTURE_ASSET_ID::EMOTION_BAR,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    registry.renderRequests.insert(npc_emotion_bar, npc_emotion_bar_request);

    int intimidate_threshold = -npc_stats.reputation;
    int charm_threshold = npc_stats.cuteness;

    float total_range = charm_threshold - intimidate_threshold;

    float relative_position = (0 - intimidate_threshold) / total_range;

    Stats& player_stat = registry.stats.get(currentPlayer);
    float bar_ticker_x = 0.0;
    if(player_stat.intelligence > 10) {
        bar_ticker_x = emotion_bar_x - (max_emotion_bar_width / 2) + (relative_position * max_emotion_bar_width);
    } else {
        bar_ticker_x = window_width_px/2.0f;
    }
    float bar_ticker_y = emotion_bar_y;

    float bar_ticker_width = max_emotion_bar_width/10.5f;

    if(!registry.motions.has(emotion_bar_ticker)){
        registry.motions.emplace(emotion_bar_ticker);
    }
    Motion& bar_ticker_motion = registry.motions.get(emotion_bar_ticker);
    bar_ticker_motion.position = {bar_ticker_x, bar_ticker_y};
    bar_ticker_motion.scale = {bar_ticker_width, emotion_bar_motion.scale.y/4.0f};
    bar_ticker_motion.velocity = {0, 0};
    bar_ticker_motion.z = 1;

    RenderRequest emotion_bar_ticker_request = {
        TEXTURE_ASSET_ID::EMOTION_BAR_TICKER,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    registry.uiElements.emplace(emotion_bar_ticker);
    registry.renderRequests.insert(emotion_bar_ticker, emotion_bar_ticker_request);
}

void EncounterSystem::renderNPCEmotionText(TextRenderer* textRenderer, Stats npc_stats, Entity npc_emotion_bar) {
    Motion& npc_eb_motion = registry.motions.get(npc_emotion_bar);
    float text_scale = (npc_eb_motion.scale.x / 3.0f) / 375.0f;
    GLfloat text_y = window_height_px - (window_height_px/6.0f) * 2.22f;

    GLfloat int_text_x = npc_eb_motion.position.x - npc_eb_motion.scale.x * 0.85f;

    textRenderer->RenderText("Intimidate", int_text_x, text_y, text_scale, glm::vec3(1.0f, 1.0f, 1.0f));

    GLfloat charm_text_x = npc_eb_motion.position.x + npc_eb_motion.scale.x * 0.5f;
    textRenderer->RenderText("Charm", charm_text_x, text_y, text_scale, glm::vec3(1.0f, 1.0f, 1.0f));
}

void EncounterSystem::renderEncounterTextBox(Entity text_box) {
    RenderRequest textbox_render_request = {
        TEXTURE_ASSET_ID::TEXT_BOX,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    registry.renderRequests.insert(text_box, textbox_render_request);
}

void EncounterSystem::renderAttackBox(Entity encounter_attack_box) {
    RenderRequest attack_box_request = {
        TEXTURE_ASSET_ID::ATTACK_BOX,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    registry.renderRequests.insert(encounter_attack_box, attack_box_request);
}

void EncounterSystem::renderEncounterText(TextRenderer *textRenderer, std::string& message, Entity text_box) {
    float base_text_scale = 0.0004;
    float text_scale = base_text_scale * window_height_px;
    Motion& text_box_m = registry.motions.get(text_box);

    GLfloat box_width = text_box_m.scale.x * 0.8f;  // 80% of the box's width
    GLfloat box_height = text_box_m.scale.y * 0.8f; // 80% of the box's height

    // Adjust to render from the top left corner of the box
    GLfloat text_x = text_box_m.position.x - box_width / 2.0f;
    GLfloat text_y = window_height_px - text_box_m.position.y + box_height * 0.4; // Start near the top edge

    // Render the text inside the box
    textRenderer->RenderBoxedText(message, text_x, text_y, text_scale, glm::vec3(1.0f, 1.0f, 1.0f), box_width, box_height);
}

void EncounterSystem::updateStatsVisuals(Stats npc_stats, Entity npc_emotion_bar, Entity emotion_bar_ticker, Entity npc_health_bar) {
    //Update health bar
    if(npc_last_health != npc_stats.currentHp) {
        npc_last_health = npc_stats.currentHp;

        Motion& health_bar_motion = registry.motions.get(npc_health_bar);

        float remaining_hp_percent = (float)npc_stats.currentHp/(float)npc_stats.maxHp;
        float max_health_bar_width_percent = 0.1f;
        float max_health_bar_width = window_width_px * max_health_bar_width_percent;

        float new_width = max_health_bar_width * remaining_hp_percent;

        float width_difference = health_bar_motion.scale.x - new_width;
        health_bar_motion.position.x -= width_difference / 2.2f;
        health_bar_motion.scale.x = max_health_bar_width * remaining_hp_percent;
    }

    //Update emotion bar
    if(npc_last_emotion != npcEmotion) {
        npc_last_emotion = npcEmotion;

        Stats& player_stats = registry.stats.get(currentPlayer);
        Motion& bar_ticker_motion = registry.motions.get(emotion_bar_ticker);
        if(player_stats.intelligence > 10) {
            int intimidate_threshold = -npc_stats.reputation;
            int charm_threshold = npc_stats.cuteness;

            float total_range = charm_threshold - intimidate_threshold;

            float relative_position = (npcEmotion - intimidate_threshold) / total_range;

            Motion& emotion_bar_motion = registry.motions.get(npc_emotion_bar);
            float ticker_x = emotion_bar_motion.position.x - (emotion_bar_motion.scale.x / 2) + (relative_position * emotion_bar_motion.scale.x);

            bar_ticker_motion.position.x = ticker_x;
        } else {
            float normalized_emotion = getNormalizedEmotion();

            float emotion_bar_width_percent = 0.15f;
            float emotion_bar_width = window_width_px * emotion_bar_width_percent;

            float offset = (emotion_bar_width / 2.0f) * normalized_emotion;

            bar_ticker_motion.position.x = window_width_px / 2.0f + offset;
        }
    }
}

void EncounterSystem::renderActionButtonText(TextRenderer *textRenderer) {

    float text_scale = 0.7f;

    // ATTACK BUTTON
    GLfloat attack_text_x = window_width_px/14;
    GLfloat attack_text_y = window_height_px/6.2f;

    textRenderer->RenderText("Attack" , attack_text_x, attack_text_y, text_scale, glm::vec3(1.0f, 1.0f, 1.0f));


    // CHARM BUTTON
    GLfloat charm_text_x = window_width_px/7.5 + window_width_px/3.75;
    GLfloat charm_text_y = window_height_px/6.2f;

    textRenderer->RenderText("Charm" , charm_text_x, charm_text_y, text_scale, glm::vec3(1.0f, 1.0f, 1.0f));

    // INTIMIDATE BUTTON
    GLfloat int_text_x = 2*(window_width_px/7.5) + window_width_px/2.15;
    GLfloat int_text_y = window_height_px/6.2f;

    textRenderer->RenderText("Intimidate" , int_text_x, int_text_y, text_scale, glm::vec3(1.0f, 1.0f, 1.0f));

}


std::string EncounterSystem::encounterStatusToString(ENCOUNTER_STATUS status) {
    
    {
        switch (status) {
            case ENCOUNTER_STATUS::DEFAULT: return "DEFAULT";
            case ENCOUNTER_STATUS::WAITING_FOR_PLAYER_ACTION: return "WAITING_FOR_PLAYER_ACTION";
            case ENCOUNTER_STATUS::NPC_ACTION: return "NPC_ACTION";
            case ENCOUNTER_STATUS::PLAYER_ACTION: return "PLAYER_ACTION";
            case ENCOUNTER_STATUS::LOST: return "LOST";
            case ENCOUNTER_STATUS::W_KNOCKOUT: return "W_KNOCKOUT";
            case ENCOUNTER_STATUS::W_CHARMED: return "W_CHARMED";
            case ENCOUNTER_STATUS::W_INTIMIDATED: return "W_INTIMIDATED";
            case ENCOUNTER_STATUS::TYPES_SIZE: return "TYPES_SIZE";
            default: return "UNKNOWN";
        }
    }
}

void EncounterSystem::renderCatAttack(Entity cat_attack) {
    RenderRequest cat_render_request = {
        TEXTURE_ASSET_ID::CAT_ATTACK,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };
    if(!registry.renderRequests.has(cat_attack)){
        registry.renderRequests.insert(cat_attack, cat_render_request);
    }
}

float EncounterSystem::calculateProjectileSpawnInterval(Stats npcStat) {
    float npcFerocity = npcStat.ferocity;
    const float minInterval = 0.6f;
    const float maxInterval = 0.8f;

    if(!tutorialComplete && registry.npcs.get(currentNpc).isTutorialNPC) {
        return 1.1f;
    }

    return maxInterval - (npcFerocity / 100.0f) * (maxInterval - minInterval);
}



void EncounterSystem::createProjectile() {
    std::uniform_int_distribution<int> projectilePos(0,3);
    std::uniform_real_distribution<float> percent(0,1);
    Entity projectile = registry.create_entity();

    float boxLeft = attack_box_x - attack_box_width/2.0f;
    float boxRight = attack_box_x + attack_box_width/2.0f;
    float boxTop = attack_box_y - attack_box_width/2.0f;
    float boxBottom = attack_box_y + attack_box_width/2.0f;

    int spawnSide = projectilePos(rng);

    vec2 spawnPosition;
    vec2 direction;

    float posPercent = percent(rng);

    switch(spawnSide) {
        case 0:
            //std::cout << "Left spawn" << std::endl;
            spawnPosition = {boxLeft, posPercent * (boxBottom - boxTop) + boxTop};
            break;
        case 1:
            //std::cout << "Top spawn" << std::endl;
            spawnPosition = {posPercent * (boxRight - boxLeft) + boxLeft, boxTop};
            break;
        case 2:
            //std::cout << "Right spawn" << std::endl;
            spawnPosition = {boxRight, posPercent * (boxBottom - boxTop) + boxTop};
            break;
        case 3:
            //std::cout << "Bottom spawn" << std::endl;
            spawnPosition = {posPercent * (boxRight - boxLeft) + boxLeft, boxBottom};
            break;
        default:
            break;
    };

    vec2 center = {attack_box_x, attack_box_y};
    direction = center - spawnPosition;

    float length = sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0.0f) {
        direction.x /= length;
        direction.y /= length;
    }

    Motion& motion = registry.motions.emplace(projectile);
    motion.position = spawnPosition;

    if(!tutorialComplete) {
        motion.velocity = direction * 90.0f;
    } else {
        motion.velocity = direction * 120.0f;
    }
    motion.scale = {attack_box_width/9.0f, attack_box_width/9.0f};
    motion.angle = atan2(direction.y, direction.x);

    BoundingBox& bb = registry.boundingBoxes.emplace(projectile);
    bb.height = attack_box_width/9.0f;
    bb.width = attack_box_width/9.0f;

    RenderRequest renderRequest = {
        TEXTURE_ASSET_ID::NPC_PROJECTILE,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    registry.renderRequests.insert(projectile, renderRequest);

    activeColliders.push_back(projectile);
}

void EncounterSystem::updateNPCAction(float elapsed_ms) {
    if(!npcAttacking) {
        return;
    }

    float elapsed_seconds = elapsed_ms/1000.0f;
    timeSinceLastSpawn += elapsed_seconds;
    timeSinceAttackStarted += elapsed_seconds;

    if(damageCooldown > 0.0f) {
        damageCooldown -= elapsed_seconds;
        damageFlickerTimer += elapsed_seconds;
    }

    if(npcAttackPattern == 1) {
        if(timeSinceAttackStarted >= 15.0f) {
            npcAttacking = false;
            return;
        }
    } else {
        if(timeSinceAttackStarted >= 17.0f) {  // Longer attack duration for wave attack
            npcAttacking = false;
            return;
        }
    }

    if(timeSinceAttackStarted >= 0.5f) {
        if(npcAttackPattern == 1) {
            updateProjectiles(elapsed_ms);

            if(timeSinceLastSpawn >= spawnInterval) {
                createProjectile();
                timeSinceLastSpawn = 0.0f;
            }

        } else if(npcAttackPattern == 2) {
            updateWaveWalls(elapsed_ms);
            
            if(timeSinceLastSpawn >= spawnInterval) {
                createWaveWall();
                timeSinceLastSpawn = 0.0f;
            }
        } else if(npcAttackPattern == 3) {
            updateBounceProjectiles(elapsed_ms);
            if(timeSinceLastSpawn >= spawnInterval && activeColliders.size() < maxBounceProjectiles) {
                createBounceProjectile();
                timeSinceLastSpawn = 0.0f;
            }
        }
    }
}

void EncounterSystem::updateProjectiles(float elapsed_ms) {
    std::vector<Entity> projectilesToRemove;

    for(Entity projectile: activeColliders) {
        if(!registry.motions.has(projectile)){
            continue;
        }
        Motion& motion = registry.motions.get(projectile);

        motion.position += motion.velocity * (elapsed_ms/1000.0f);

        if(motion.position.x < attack_box_x - attack_box_width / 2.0f || 
        motion.position.x > attack_box_x + attack_box_width / 2.0f || 
        motion.position.y < attack_box_y - attack_box_width / 2.0f || 
        motion.position.y > attack_box_x + attack_box_width / 2.0f) {
            registry.remove_all_components_of(projectile);
            projectilesToRemove.push_back(projectile);
        }
    }

    for(Entity projectile: projectilesToRemove) {
        activeColliders.erase(std::remove(activeColliders.begin(), activeColliders.end(), projectile),activeColliders.end());
    }
}

void EncounterSystem::checkEncounterCollision(Entity cat_attack) {
    if(!registry.motions.has(cat_attack)) {
        return;
    }

    if (damageCooldown > 0.0f) {
        if(damageFlickerTimer >= damageFlickerInterval) {
            if(registry.renderRequests.has(cat_attack)) {
                registry.renderRequests.remove(cat_attack);
            } else {
                renderCatAttack(cat_attack);
            }

            damageFlickerTimer = 0.0f;
        }
        return;
    }

    if(!registry.renderRequests.has(cat_attack)) {
        renderCatAttack(cat_attack);
    }

    Motion& catMotion = registry.motions.get(cat_attack);
    BoundingBox& catBB = registry.boundingBoxes.get(cat_attack);

    for(Entity collider: activeColliders) {
        if (!registry.motions.has(collider) || !registry.boundingBoxes.has(collider)) {
            continue; 
        }

        Motion& collMotion = registry.motions.get(collider);
        BoundingBox& collBB = registry.boundingBoxes.get(collider);

        bool isCollision = 
            catMotion.position.x - catBB.width / 2.0f < collMotion.position.x + collBB.width / 2.0f &&
            catMotion.position.x + catBB.width / 2.0f > collMotion.position.x - collBB.width / 2.0f &&
            catMotion.position.y - catBB.height / 2.0f < collMotion.position.y + collBB.height / 2.0f &&
            catMotion.position.y + catBB.height / 2.0f > collMotion.position.y - collBB.height / 2.0f;

        if(isCollision) {
            Mix_PlayChannel(2, p_hurt_sound, 0);
            Stats& playerStats = registry.stats.get(currentPlayer);

            // can't die during tutorial
            if(!tutorialComplete && registry.npcs.get(currentNpc).isTutorialNPC) {
                if(playerStats.currentHp == 1) {
                    playerStats.currentHp = 1;
                } else {
                    playerStats.currentHp -= 1;
                }
            } else {
                playerStats.currentHp -= 1;
            }

            if(playerStats.currentHp <= 0) {
                playerStats.currentHp = 0;
                setStatus(ENCOUNTER_STATUS::LOST);
            }

            damageCooldown = damageCooldownDuration;

            if(npcAttackPattern == 1 || npcAttackPattern == 3) { // Projectile attack patterns
                registry.remove_all_components_of(collider);
                activeColliders.erase(std::remove(activeColliders.begin(), activeColliders.end(), collider), activeColliders.end());
            }
        }
    }
}

float EncounterSystem::calculateWaveVelocity(Stats npcStat) {
    float npcFerocity = npcStat.ferocity;
    float minVelocity = 150.0f;
    float maxVelocity = 250.0f;

    if(!tutorialComplete && registry.npcs.get(currentNpc).isTutorialNPC) {
        return 100.0f; // Slower speed for tutorial
    }

    return minVelocity + (npcFerocity/100.0f) * (maxVelocity - minVelocity);
}

float EncounterSystem::calculateWaveSpawnInterval(Stats npcStat) {
    float npcFerocity = npcStat.ferocity;
    float minInterval = 1.0f;
    float maxInterval = 1.8f;

    if(!tutorialComplete && registry.npcs.get(currentNpc).isTutorialNPC) {
        return 2.3f;
    }

    return maxInterval - (npcFerocity/100.0f) * (maxInterval - minInterval);
}

void EncounterSystem::createWaveWall() {
    std::uniform_int_distribution<int> patternDist(1, 3);
    int pattern = patternDist(rng);

    float spawnX = attack_box_x + attack_box_width / 2.0f;
    float boxHeight = attack_box_width;
    float boxTop = attack_box_y - boxHeight / 2.0f;
    float boxBottom = attack_box_y + boxHeight / 2.0f;
    switch(pattern) {
        case 1: {
            std::uniform_real_distribution<float> lengthDist(boxHeight / 3.0f, attack_box_width - attack_box_width/7.0f);
            float topWallLength = lengthDist(rng);

            Entity topWall = registry.create_entity();
            Motion& topWallMotion = registry.motions.emplace(topWall);
            topWallMotion.position = {spawnX, boxTop + topWallLength / 2.0f};
            topWallMotion.scale = {attack_box_width / 25.0f, topWallLength};
            topWallMotion.velocity = {-waveVelocity, 0};

            BoundingBox& topBB = registry.boundingBoxes.emplace(topWall);
            topBB.width = attack_box_width / 25.0f;
            topBB.height = topWallLength;

            RenderRequest topWallRenderRequest = {
                TEXTURE_ASSET_ID::ENCOUNTER_WALL,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                {}};

            registry.renderRequests.insert(topWall, topWallRenderRequest);
            activeColliders.push_back(topWall);
            break;
        }
        case 2: {
            std::uniform_real_distribution<float> lengthDist(boxHeight / 3.0f, attack_box_width - attack_box_width/7.0f);
            float bottomWallLength = lengthDist(rng);

            Entity bottomWall = registry.create_entity();
            Motion& bottomWallMotion = registry.motions.emplace(bottomWall);
            bottomWallMotion.position = {spawnX, boxBottom - bottomWallLength / 2.0f};
            bottomWallMotion.scale = {attack_box_width / 25.0f, bottomWallLength};
            bottomWallMotion.velocity = {-waveVelocity, 0};

            BoundingBox& bottomBB = registry.boundingBoxes.emplace(bottomWall);
            bottomBB.width = attack_box_width / 25.0f;
            bottomBB.height = bottomWallLength;

            RenderRequest bottomWallRenderRequest = {
                TEXTURE_ASSET_ID::ENCOUNTER_WALL,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                {}};
            registry.renderRequests.insert(bottomWall, bottomWallRenderRequest);
            activeColliders.push_back(bottomWall);
            break;
        }
        case 3: {
            float gap = attack_box_width/5.0f;
            std::uniform_real_distribution<float> lengthDist(0.0f, attack_box_width - gap);

            float topWallLength = lengthDist(rng);
            float bottomWallLength = boxHeight - gap - topWallLength;
            if(bottomWallLength < 0) {
                bottomWallLength = 0;
            }

            // Top Wall
            Entity topWall = registry.create_entity();
            Motion& topWallMotion = registry.motions.emplace(topWall);
            topWallMotion.position = {spawnX, boxTop + topWallLength / 2.0f};
            topWallMotion.scale = {attack_box_width / 25.0f, topWallLength};
            topWallMotion.velocity = {-waveVelocity, 0};

            BoundingBox& topBB = registry.boundingBoxes.emplace(topWall);
            topBB.width = attack_box_width / 25.0f;
            topBB.height = topWallLength;

            RenderRequest topWallRenderRequest = {
                TEXTURE_ASSET_ID::ENCOUNTER_WALL,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                {}};
            registry.renderRequests.insert(topWall, topWallRenderRequest);
            activeColliders.push_back(topWall);

            // Bottom Wall
            Entity bottomWall = registry.create_entity();
            Motion& bottomWallMotion = registry.motions.emplace(bottomWall);
            bottomWallMotion.position = {spawnX, boxBottom - bottomWallLength / 2.0f};
            bottomWallMotion.scale = {attack_box_width / 25.0f, bottomWallLength};
            bottomWallMotion.velocity = {-waveVelocity, 0};

            BoundingBox& bottomBB = registry.boundingBoxes.emplace(bottomWall);
            bottomBB.width = attack_box_width / 25.0f;
            bottomBB.height = bottomWallLength;

            RenderRequest bottomWallRenderRequest = {
                TEXTURE_ASSET_ID::ENCOUNTER_WALL,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                {}};
            registry.renderRequests.insert(bottomWall, bottomWallRenderRequest);
            activeColliders.push_back(bottomWall);
            break;
        }
        default:
            break;
    }
}

void EncounterSystem::updateWaveWalls(float elapsed_ms) {
    std::vector<Entity> wallsToRemove;
    for(Entity wall: activeColliders) {
        if(!registry.motions.has(wall)) {
            continue;
        }

        Motion& motion = registry.motions.get(wall);
        //std::cout << motion.velocity.x << std::endl;
        motion.position += motion.velocity * (elapsed_ms/1000.0f);

        if(motion.position.x < attack_box_x - attack_box_width / 2.0f) {
            registry.remove_all_components_of(wall);
            wallsToRemove.push_back(wall);
        }
    }

    for(Entity wall: wallsToRemove) {
        activeColliders.erase(std::remove(activeColliders.begin(), activeColliders.end(), wall),activeColliders.end());
    }
}

void EncounterSystem::createBounceProjectile() {
    Entity projectile = registry.create_entity();

    Motion& motion = registry.motions.emplace(projectile);
    motion.position = {attack_box_x + attack_box_width/2.0f - attack_box_width/18.0f - attack_box_width*0.05, attack_box_y};
    motion.scale = {attack_box_width/9.0f, attack_box_width/9.0f};

    const float epsilon = 0.1f; // Slight offset to avoid straight up/down
    std::uniform_real_distribution<float> angleDist(M_PI / 2 + epsilon, 3 * M_PI / 2 - epsilon);
    motion.angle = angleDist(rng);

    if(!tutorialComplete) {
        motion.velocity = {80.0f, 80.0f};
    } else {
        motion.velocity = {100.0f, 100.0f};
    }

    BoundingBox& bb = registry.boundingBoxes.emplace(projectile);
    bb.height = attack_box_width/9.0f;
    bb.width = attack_box_width/9.0f;

    RenderRequest renderRequest = {
        TEXTURE_ASSET_ID::BOUNCE_PROJECTILE,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    registry.renderRequests.insert(projectile, renderRequest);

    activeColliders.push_back(projectile);
}

void EncounterSystem::updateBounceProjectiles(float elapsed_ms) {
    float elapsed_seconds = elapsed_ms / 1000.0f;

    for(Entity projectile: activeColliders) {
        if(!registry.motions.has(projectile)){
            continue;
        }
        Motion& motion = registry.motions.get(projectile);

        motion.position.x += motion.velocity.x * cos(motion.angle) * elapsed_seconds;
        motion.position.y += motion.velocity.y * sin(motion.angle) * elapsed_seconds;

        if (motion.position.x - motion.scale.x / 2.0f < attack_box_x - attack_box_width / 2.0f ||
            motion.position.x + motion.scale.x / 2.0f > attack_box_x + attack_box_width / 2.0f) {
            motion.angle = M_PI - motion.angle;
        }

        if (motion.position.y - motion.scale.y / 2.0f < attack_box_y - attack_box_width / 2.0f ||
            motion.position.y + motion.scale.y / 2.0f > attack_box_y + attack_box_width / 2.0f) {
            motion.angle = -motion.angle;
        }
    }
}

void EncounterSystem::renderPlayerStats(TextRenderer* textRenderer, Stats& stats){
    float textSize = 0.5;
    GLfloat textX = 10;
    GLfloat textY = window_height_px - 30;

    std::string attackText = "Attack +" + stat_to_string(stats.ferocity);
    std::string charmText = "Charm +" + stat_to_string(stats.cuteness);
    std::string intimidateText = "Intimidate +" + stat_to_string(stats.reputation);

    textRenderer->RenderText(attackText, textX, textY, textSize, {1, 1,1});
    textRenderer->RenderText(charmText, textX, textY - 30, textSize, {1, 1,1});
    textRenderer->RenderText(intimidateText, textX, textY - 60, textSize, {1, 1,1});
}