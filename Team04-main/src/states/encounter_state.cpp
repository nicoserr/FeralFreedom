#include "encounter_state.hpp"
#include "start_state.hpp"
#include "../world/world_system.hpp"
#include "../systems/encounter_system.hpp"
#include <iostream>
#include <string>

#include "game_over_state.hpp"
#include "play_state.hpp"
#include "../core/ecs_registry.hpp"
#include "serialization/registry_serializer.hpp"

Entity EncounterState::player_entity;
Entity EncounterState::npc_entity;
Entity EncounterState::npc_health_bar;
Entity EncounterState::npc_emotion_bar;
Entity EncounterState::npc_emotion_bar_ticker;
Entity EncounterState::encounter_text_box;
Entity EncounterState::encounter_attack_box;
Entity EncounterState::cat_attack;
Stats EncounterState::player_stats;
Stats EncounterState::npc_stats;
int EncounterState::current_player_hp;
int EncounterState::player_index;
int EncounterState::npc_texture_id;
std::string EncounterState::encounter_message;
std::string EncounterState::npc_name;
EncounterSystem EncounterState::encounter_system;
float EncounterState::ms_since_status_update;
Gesture EncounterState::line;
bool EncounterState::wasMouseClicked;
Gesture EncounterState::triangle;
Gesture EncounterState::zigzag;
Gesture EncounterState::currentTarget;
float EncounterState::currentT;
float EncounterState::step;
Gesture EncounterState::bezierTarget;
bool EncounterState::isTutorial;

/***
 * TODO:
 * - Game over screen.
 */

EncounterState* EncounterState::instance(Stats player, Stats npc, int p_id, int texture_id, std::string n_name, bool isTutorialNPC) {
    static EncounterState instance;
    player_entity = registry.create_entity();
    npc_entity = registry.create_entity();
    npc_health_bar = registry.create_entity();
    npc_emotion_bar = registry.create_entity();
    encounter_text_box = registry.create_entity();
    encounter_text_box = registry.create_entity();
    encounter_attack_box = registry.create_entity();
    cat_attack = registry.create_entity();
    

    wasMouseClicked = false;
    player_stats = player;
    npc_stats = npc;
    player_index = p_id;
    npc_texture_id = texture_id;
    npc_name = n_name;
    ms_since_status_update = 0;
    currentT = 0;
    step = 0.1;
    isTutorial = isTutorialNPC;
    Mix_Volume(1, 7);
    Mix_Volume(2, 20);
    return &instance;

}

void EncounterState::init(WorldSystem* game) {
    // printf("Encounter State initialized\n");    
    displayEndScreen = false;

    Stats& player = registry.stats.insert(player_entity, player_stats);
    registry.stats.insert(npc_entity, npc_stats);

    // Initialize current HP
    current_player_hp = player.currentHp;

    registry.npcs.emplace(npc_entity);
    NPC& npc_info = registry.npcs.get(npc_entity);
    npc_info.name = npc_name;
    npc_info.encounter_texture_id = npc_texture_id;
    npc_info.isTutorialNPC = isTutorial;

    renderer = game->get_renderer();
    textRenderer = game->get_text_renderer();
    shadowRenderer = game->get_shadow_renderer();

    // Create and render buttons
    // TODO: fix x positions for buttons
    // TODO: fix y detection for buttons
    // Attack Button
    myButton attackButton = createButton(
        vec2(window_width_px/6 , window_height_px - (window_height_px/6.0f)),
        vec2(window_width_px/4.5, window_height_px/7.5),
        TEXTURE_ASSET_ID::ACTION_BUTTON,
        BUTTON_TYPE::ATTACK
    );
    buttons.push_back(attackButton);
    registry.uiElements.emplace(attackButton.entity);

    // Charm Button
    myButton charmButton = createButton(
        vec2(window_width_px/6 + window_width_px/3, window_height_px - (window_height_px/6.0f)),
        vec2(window_width_px/4.5, window_height_px/7.5),
        TEXTURE_ASSET_ID::ACTION_BUTTON,
        BUTTON_TYPE::CHARM
    );
    buttons.push_back(charmButton);
    registry.uiElements.emplace(charmButton.entity);

    // Intimidate Button
    myButton intimidateButton = createButton(
        vec2(window_width_px/6 + 2*window_width_px/3, window_height_px - (window_height_px/6.0f)),
        vec2(window_width_px/4.5, window_height_px/7.5),
        TEXTURE_ASSET_ID::ACTION_BUTTON,
        BUTTON_TYPE::INTIMIDATE
    );
    buttons.push_back(intimidateButton);
    registry.uiElements.emplace(intimidateButton.entity);

    encounter_system.initEncounter(player_entity, npc_entity);
    encounter_system.createEncounter(npc_texture_id, npc_name, npc_entity, npc_stats, npc_health_bar, 
    npc_emotion_bar, npc_emotion_bar_ticker, encounter_text_box, encounter_attack_box, renderer, textRenderer, cat_attack);

    Motion& attackBoxMotion = registry.motions.get(encounter_attack_box);
    line.points.clear();
    line.points.push_back({attackBoxMotion.position.x - attackBoxMotion.scale.x/2, 
    attackBoxMotion.position.y - attackBoxMotion.scale.y/2});
    line.points.push_back({attackBoxMotion.position.x,attackBoxMotion.position.y});
    line.points.push_back({attackBoxMotion.position.x + attackBoxMotion.scale.x / 2,
    attackBoxMotion.position.y + attackBoxMotion.scale.y / 2});

    vec2 center = {0,0};
    for (const auto& point : line.points){
        center += point;
    }

    center /= static_cast<float>(line.points.size());

    for(auto& point : line.points){
        point = center + 0.7f * (point - center);
    }


    triangle.points.clear();
    triangle.points.push_back({attackBoxMotion.position.x - attackBoxMotion.scale.x/2,  
    attackBoxMotion.position.y});
    triangle.points.push_back({attackBoxMotion.position.x,attackBoxMotion.position.y + attackBoxMotion.scale.y / 2});
    triangle.points.push_back({attackBoxMotion.position.x + attackBoxMotion.scale.x / 2, 
    attackBoxMotion.position.y});

    center = {0,0};
    for (const auto& point : triangle.points){
        center += point;
    }

    center /= static_cast<float>(triangle.points.size());


    for(auto& point : triangle.points){
        point = center + 0.7f * (point - center);
    }


    zigzag.points.clear();
    zigzag.points.push_back({attackBoxMotion.position.x - attackBoxMotion.scale.x/2, 
    attackBoxMotion.position.y - attackBoxMotion.scale.y/2});
    zigzag.points.push_back({attackBoxMotion.position.x - attackBoxMotion.scale.x/4, 
    attackBoxMotion.position.y + attackBoxMotion.scale.y/2});
    zigzag.points.push_back({attackBoxMotion.position.x, 
    attackBoxMotion.position.y - attackBoxMotion.scale.y/2});
    zigzag.points.push_back({attackBoxMotion.position.x + attackBoxMotion.scale.x / 2,
    attackBoxMotion.position.y + attackBoxMotion.scale.y / 2});

    center = {0,0};
    for (const auto& point : zigzag.points){
        center += point;
    }

    center /= static_cast<float>(zigzag.points.size());


    for(auto& point : zigzag.points){
        point = center + 0.7f * (point - center);
    }


    // TODO: Need player entity and rendering
    // ENCOUNTER SOUNDS
    encounter_system.encounter_music = Mix_LoadWAV(audio_path("encounter.wav").c_str());
    // Player Action Sounds
    encounter_system.p_attack_sound = Mix_LoadWAV(audio_path("p_attack.wav").c_str());
    encounter_system.p_charm_sound = Mix_LoadWAV(audio_path("p_charm.wav").c_str());
    encounter_system.p_intimidate_sound = Mix_LoadWAV(audio_path("p_intimidate.wav").c_str());

    // NPC Attack Sound
    encounter_system.npc_attack_sound = Mix_LoadWAV(audio_path("npc_attack.wav").c_str());
    encounter_system.p_hurt_sound = Mix_LoadWAV(audio_path("p_hurt.wav").c_str());

    if (encounter_system.p_attack_sound == nullptr || encounter_system.p_charm_sound == nullptr || encounter_system.p_intimidate_sound == nullptr || encounter_system.npc_attack_sound == nullptr || encounter_system.p_hurt_sound == nullptr || encounter_system.encounter_music == nullptr)
    {
        fprintf(stderr, "Failed to load encounter sounds\n %s,\n %s,\n %s,\n %s,\n %s\n, %s\n, make sure the data directory is present\n",
            audio_path("attack1.wav").c_str(),
            audio_path("charm1.wav").c_str(),
            audio_path("intimidate1.wav").c_str(),
            audio_path("npc_attack.wav").c_str(),
            audio_path("p_hurt.wav").c_str(),
            audio_path("encounter.wav").c_str(),
            Mix_GetError());
    }
    Mix_Volume(1, 7);
    Mix_FadeInChannel(1, encounter_system.encounter_music, -1, 1000);
}

void EncounterState::cleanup(WorldSystem* game) {
    // printf("Cleaning up Encounter state.\n");
    std::string state_name = typeid(*this).name();
    // Save the current state before cleaning up
    // printf("Saving registry state for %s...\n", stateName.c_str());

    // registry.list_all_components();
    encounter_system.attacking = false;
    encounter_system.npcAttacking = false;
    // RegistrySerializer::cleanupStateFile(state_name);
    if (RegistrySerializer::encounterUpdateStateFile(registry.stats.get(player_entity),
        registry.stats.get(npc_entity),
        player_index))
    {
        // printf("Successfully updated state file.\n");
    }
    else {
        printf("State file not updated.\n");
    }

    //printf("Registry state before cleanup:\n");
    // printf("Number of entities in registry: %zu\n", registry.get_entities().size());

    registry.get_entities().clear(); // we need this or else it doesn't clean the entire entity list
    registry.clear_all_components();
    // if i do this i get // Assertion failed: (!(check_for_duplicates && has(e)) && "Entity already contained in ECS registry"), function insert, file ecs.hpp, line 62.
    // game->get_level_manager()->cleanUpAllRooms();
    // with this i get // Assertion failed: (has(e) && "Entity not contained in ECS registry"), function get, file ecs.hpp, line 93.

    // TODO: uncomment ?
    // game->get_level_manager()->cleanUpCurrentRoom();

    player_character = Entity();

    // Cleanup Sound
    if (encounter_system.p_attack_sound != nullptr)
        Mix_FreeChunk(encounter_system.p_attack_sound);
    if (encounter_system.p_charm_sound != nullptr)
        Mix_FreeChunk(encounter_system.p_charm_sound);
    if (encounter_system.p_intimidate_sound != nullptr)
        Mix_FreeChunk(encounter_system.p_intimidate_sound);
    if (encounter_system.npc_attack_sound != nullptr)
        Mix_FreeChunk(encounter_system.npc_attack_sound);
    if (encounter_system.p_hurt_sound != nullptr)
        Mix_FreeChunk(encounter_system.p_hurt_sound);
    if (encounter_system.encounter_music != nullptr)
        Mix_FreeChunk(encounter_system.encounter_music);

    // printf("Registry state after cleanup:\n");
    // printf("Number of entities in registry: %zu\n", registry.get_entities().size());
}

void EncounterState::reset(WorldSystem *game) {
    encounter_system.attacking = false;
    encounter_system.npcAttacking = false;
}

void EncounterState::pause() {
    printf("Encounter state paused\n");
}

void EncounterState::resume() {
    printf("Encounter state resumed\n");
}

void EncounterState::handle_key_input(WorldSystem* game, int key, int scancode, int action, int mods) {
    // for now, just quit the game if escape is pressed in encounter
    if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(game->get_window(), GLFW_TRUE);
    }

    // For testing. Update once encounter system is working.
    if (key == GLFW_KEY_0 && action == GLFW_PRESS) {
        PlayState* instance = PlayState::instance();
        instance->tutorialFinished = true;
        game->change_state(instance);
    }
    /*
    if (key == GLFW_KEY_A && action == GLFW_PRESS) {
        printf("[A] PRESSED in Encounter state: removing 10 health from player\n");
        registry.stats.get(player_entity).currentHp -= 10;
        printf("Player currentHp: %i\n", registry.stats.get(player_entity).currentHp);
    }
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
         printf("[S] PRESSED in Encounter state: removing 1 agility from npc\n");
         registry.stats.get(npc_entity).agility -= 1;
         printf("NPC agility: %i\n", registry.stats.get(npc_entity).agility);
     }
    */

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        if(displayBeginningScreen)
            displayBeginningScreen = false;

        else if (displayEndScreen) {
            PlayState* instance = PlayState::instance();
            instance->tutorialFinished = true;
            game->change_state(instance);
        }
    }

    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        if(encounter_system.getStatus() == ENCOUNTER_STATUS::DIALOGUE) {
            if(!encounter_system.encounterMessages.empty()) {
                currentMessage = encounter_system.encounterMessages.front();
                encounter_system.encounterMessages.pop();
            } else {
                currentMessage = "";
            }
            return;
        }
    }
}

void EncounterState::handle_mouse_movement(WorldSystem* game, double xpos, double ypos) {
    if(encounter_system.attacking || encounter_system.npcAttacking){
        glfwSetInputMode(game->get_window(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        
        if(!registry.renderRequests.has(cat_attack)){
            encounter_system.renderCatAttack(cat_attack);
        }
        Motion* cat_motion; 
        Motion& attack_box_motion = registry.motions.get(encounter_attack_box);
        if(!registry.motions.has(cat_attack)){
            cat_motion = &registry.motions.emplace(cat_attack);
            glfwSetCursorPos(game->get_window(), attack_box_motion.position.x, attack_box_motion.position.y);

            BoundingBox& bb = registry.boundingBoxes.emplace(cat_attack);
            bb.height = window_width_px * 0.015f;
            bb.width = window_width_px * 0.015f;
        } else {
            cat_motion = &registry.motions.get(cat_attack);
        }
        cat_motion->scale = {window_width_px * 0.015f,window_width_px * 0.015f};

        float min_x = attack_box_motion.position.x - (attack_box_motion.scale.x / 2) + cat_motion->scale.x / 2;
        float max_x = attack_box_motion.position.x + (attack_box_motion.scale.x / 2) - cat_motion->scale.x / 2;
        cat_motion->position.x = clamp(static_cast<float>(xpos), min_x, max_x);
        
        float min_y = attack_box_motion.position.y - (attack_box_motion.scale.y / 2) + cat_motion->scale.y / 2;
        float max_y = attack_box_motion.position.y + (attack_box_motion.scale.y/ 2) - cat_motion->scale.y / 2;
        cat_motion->position.y = clamp(static_cast<float>(ypos), min_y, max_y);

        float clamped_x = clamp(static_cast<float>(xpos), min_x, max_x);
        float clamped_y = clamp(static_cast<float>(ypos), min_y, max_y);

        if(xpos != clamped_x){
            glfwSetCursorPos(game->get_window(), clamped_x, ypos);
        }
        if(ypos != clamped_y){
            glfwSetCursorPos(game->get_window(), xpos, clamped_y);
        }

        if(encounter_system.attacking) {
            if(glfwGetMouseButton(game->get_window(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
                if(wasMouseClicked && playerAttackStartTimer >= 250.0f){
                    addPoint({clamped_x, clamped_y});
                }
                wasMouseClicked = true;
            } 
            if(glfwGetMouseButton(game->get_window(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE){
                // currentGesturePath.clear();
                if(!currentGesturePath.empty() && playerAttackStartTimer >= 250.0f){
                    isGestureMatch(bezierTarget);
                }
            }
        }


        
    } else {
        glfwSetInputMode(game->get_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}


void EncounterState::handle_mouse_input(WorldSystem* game, double xpos, double ypos) {
    if (displayBeginningScreen) {
        return;
    }
  
    if(encounter_system.attacking){
        return;
    }
    if(encounter_system.npcAttacking){
        return;
    }

    if(encounter_system.getStatus() == ENCOUNTER_STATUS::DIALOGUE) {
        if(!encounter_system.encounterMessages.empty()) {
            currentMessage = encounter_system.encounterMessages.front();
            encounter_system.encounterMessages.pop();
        } else {
            currentMessage = "";
        }
        return;
    }

    // Check if the mouse click is inside any button
    for (const auto& button : buttons) {
        if (is_click_inside_button({xpos,ypos}, button, *game)
            && encounter_system.getStatus() == ENCOUNTER_STATUS::WAITING_FOR_PLAYER_ACTION) {
            Motion& attack_box_motion = registry.motions.get(encounter_attack_box);
            switch (button.type) {
                case BUTTON_TYPE::ATTACK:
                    if((!encounter_system.tutorialEncounterAttackComplete && !encounter_system.tutorialCharmIntimidateComplete) || 
                    (encounter_system.tutorialEncounterAttackComplete && encounter_system.tutorialCharmIntimidateComplete) || encounter_system.tutorialComplete || !registry.npcs.get(npc_entity).isTutorialNPC) {
                        chooseTarget();
                        encounter_system.doPlayerAction(ACTION_TYPE::ATTACK);
                        ms_since_status_update = 0;
                        glfwSetCursorPos(game->get_window(), attack_box_motion.position.x, attack_box_motion.position.y);
                        break;
                    }
                    break;
                case BUTTON_TYPE::CHARM:
                    if(encounter_system.tutorialEncounterAttackComplete || encounter_system.tutorialComplete || !registry.npcs.get(npc_entity).isTutorialNPC) {
                        encounter_system.doPlayerAction(ACTION_TYPE::CHARM);
                        Mix_PlayChannel(2, encounter_system.p_charm_sound, 0);
                        ms_since_status_update = 0;
                    }
                    break;
                case BUTTON_TYPE::INTIMIDATE:
                    if(encounter_system.tutorialEncounterAttackComplete || encounter_system.tutorialComplete || !registry.npcs.get(npc_entity).isTutorialNPC) {
                        encounter_system.doPlayerAction(ACTION_TYPE::INTIMIDATE);
                        Mix_PlayChannel(2, encounter_system.p_intimidate_sound, 0);
                        ms_since_status_update = 0;
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void EncounterState::update(WorldSystem *game, float elapsed_ms) {
    ms_since_status_update += elapsed_ms;
    updateCurve(elapsed_ms);
    current_player_hp = registry.stats.get(player_entity).currentHp;
    ENCOUNTER_STATUS encounter_status = encounter_system.getStatus();
    Stats curr_player_stats = registry.stats.get(player_entity);
    Stats curr_npc_stats = registry.stats.get(npc_entity);
    encounter_system.updateStatsVisuals(curr_npc_stats, npc_emotion_bar, npc_emotion_bar_ticker, npc_health_bar);
    if(encounter_system.isEncounterOver(encounter_message, curr_player_stats, curr_npc_stats)){
        NPC& npc = registry.npcs.get(npc_entity);
        npc.to_remove = true;
        npc.isInteractable = false;
    }
    // Print all non-player messages in queue, give each message MS_PER_TURN on screen.
    // if (encounter_status != ENCOUNTER_STATUS::PLAYER_ACTION && encounter_status != ENCOUNTER_STATUS::NPC_ACTION) {
    //     //std::cout << "Displaying messages" << std::endl;
    //     if (currentMessage == "" && !encounter_system.encounterMessages.empty()) {
    //         currentMessage = encounter_system.encounterMessages.front();
    //         encounter_system.encounterMessages.pop();
    //         ms_since_status_update = 0;
    //         return;
    //     } else {
    //         // if no messages, reset current message and update player stats
    //         currentMessage = "";
    //     }   
    // }
    switch (encounter_status) {
        // Active cases:
        case ENCOUNTER_STATUS::PLAYER_ACTION:
            playerAttackStartTimer += elapsed_ms;
            if(encounter_system.playerResult == PLAYER_ACTION_RESULT::HIT || encounter_system.playerResult == PLAYER_ACTION_RESULT::MISS){
                encounter_system.setStatus(ENCOUNTER_STATUS::ATTACKED);
                //std::cout << "Set status to attacked" << std::endl;
                encounter_system.playerResult = PLAYER_ACTION_RESULT::NONE;
                playerAttackStartTimer = 0.0f;
            }
            break;
        case ENCOUNTER_STATUS::NPC_ACTION:
            // Check if encounter is over (if so, status and encounter_message will be updated)
            // otherwise, continue with cycle
            if (!encounter_system.isEncounterOver(encounter_message, curr_player_stats, curr_npc_stats)) {
                //registry.remove_all_components_of(cat_attack);
            }

            if (encounter_system.npcAttacking) {
                //std::cout << "Doing enemy attack" << std::endl;
                encounter_system.updateNPCAction(elapsed_ms);
                encounter_system.checkEncounterCollision(cat_attack);
            } else {
                //std::cout << "Finished enemy attack" << std::endl;
                encounter_system.npcAttacking = false;
                encounter_system.npcAttackPattern = -1; // reset attackPattern to -1 for next cycle
                registry.renderRequests.remove(cat_attack);
                for(Entity collider: encounter_system.activeColliders) {
                    registry.remove_all_components_of(collider);
                    encounter_system.activeColliders.clear();
                }
                wasMouseClicked = false;
                encounter_system.setStatus(ENCOUNTER_STATUS::WAITING_FOR_PLAYER_ACTION);
            }
            
            break;
        // Win cases:
        case ENCOUNTER_STATUS::W_KNOCKOUT:
            if(!encounter_system.tutorialComplete) {
                encounter_system.tutorialComplete = true;
            }
            handleKnockoutWin(game);
            break;
        case ENCOUNTER_STATUS::W_CHARMED:
            if(!encounter_system.tutorialComplete) {
                encounter_system.tutorialComplete = true;
            }
            handleCutenessWin(game);
            break;
        case ENCOUNTER_STATUS::W_INTIMIDATED:
            if(!encounter_system.tutorialComplete) {
                encounter_system.tutorialComplete = true;
            }
            handleReputationWin(game);
            break;
        // Lose case:
        case ENCOUNTER_STATUS::LOST:
            handleLoss(game);
            break;
        case ENCOUNTER_STATUS::ATTACKED:
            if (!encounter_system.isEncounterOver(encounter_message, curr_player_stats, curr_npc_stats)){
                if(encounter_system.npcAttackPattern ==  -1) {
                    std::uniform_int_distribution<int> npcPatternDist(1, 3);

                    encounter_system.npcAttackPattern = npcPatternDist(encounter_system.rng);
                    //encounter_system.npcAttackPattern = 3; //1 for projectiles, 2 for wave, 3 for bounce
                }

                if(!encounter_system.tutorialEncounterAttackComplete) {
                    encounter_system.tutorialEncounterAttackComplete = true;
                }
                encounter_system.setStatus(ENCOUNTER_STATUS::DIALOGUE);
            } else {
                encounter_system.addEncounterOverMsg(curr_player_stats, curr_npc_stats);
                encounter_system.setStatus(ENCOUNTER_STATUS::DIALOGUE);
            }
            break;
        case ENCOUNTER_STATUS::DIALOGUE:
            //std::cout << "Waiting to finish dialogue" << std::endl;
            if(!encounter_system.encounterMessages.empty() && currentMessage == "") {
                currentMessage = encounter_system.encounterMessages.front();
                encounter_system.encounterMessages.pop();
            }
            if(currentMessage == "") {
                if(registry.renderRequests.has(e_press)) {
                    registry.renderRequests.remove(e_press);
                }
                if(encounter_system.isEncounterOver(encounter_message, curr_player_stats, curr_npc_stats)) {

                    encounter_system.setEncounterOverStatus(curr_player_stats, curr_npc_stats);
                } else {
                    if((!encounter_system.tutorialEncounterAttackComplete || !encounter_system.tutorialNPCAttackComplete || !encounter_system.tutorialCharmIntimidateComplete) && registry.npcs.get(npc_entity).isTutorialNPC) {
                        if(!encounter_system.tutorialEncounterAttackComplete) {
                            encounter_system.setStatus(ENCOUNTER_STATUS::WAITING_FOR_PLAYER_ACTION);
                            break;
                        }

                        if(!encounter_system.tutorialNPCAttackComplete) {
                            encounter_system.doNPCAction(ACTION_TYPE::ATTACK);
                            break;
                        }

                        if(!encounter_system.tutorialCharmIntimidateComplete) {
                            encounter_system.setStatus(ENCOUNTER_STATUS::WAITING_FOR_PLAYER_ACTION);
                            break;
                        }
                    } else {
                        encounter_system.doNPCAction(ACTION_TYPE::ATTACK);
                    }
                }
            }
            break;
        default:
            break;
    }

    ms_since_status_update = 0;

    if(encounter_status == ENCOUNTER_STATUS::NPC_ACTION) {
        if (encounter_system.npcAttacking) {
            //std::cout << "Doing enemy attack" << std::endl;
            encounter_system.updateNPCAction(elapsed_ms);
            encounter_system.checkEncounterCollision(cat_attack);
        } else {
            encounter_system.npcAttacking = false;
            encounter_system.npcAttackPattern = -1; // reset attackPattern to -1 for next cycle
            registry.renderRequests.remove(cat_attack);
            for(Entity projectile: encounter_system.activeColliders) {
                registry.remove_all_components_of(projectile);
                encounter_system.activeColliders.clear();
            }
            wasMouseClicked = false;
            renderMisc();
            if(!encounter_system.tutorialNPCAttackComplete) {
                encounter_system.tutorialNPCAttackComplete = true;
            }

            if(!encounter_system.tutorialCharmIntimidateComplete && registry.npcs.get(npc_entity).isTutorialNPC) {
                encounter_system.encounterMessages.push("Raccoon: Enemies could have different attack patterns which will be easier or harder based on their stats");
                encounter_system.encounterMessages.push("Raccoon: Now, let's try to use Charm or Intimidate");
                encounter_system.encounterMessages.push("Raccoon: Try to select one of them below");
                if(!encounter_system.tutorialNPCAttackComplete) {
                    encounter_system.tutorialNPCAttackComplete = true;
                }
                encounter_system.setStatus(ENCOUNTER_STATUS::DIALOGUE);
            } else {
                encounter_system.setStatus(ENCOUNTER_STATUS::WAITING_FOR_PLAYER_ACTION);
            }
        }
    }

    
}

void EncounterState::draw(WorldSystem *game, float elapsed_ms_since_last_update) {
    // Clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // clean to black screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Stats curr_stats = registry.stats.get(npc_entity);
    //Stats my_stats = registry.stats.get(registry.players.entities[0]);
    if (displayBeginningScreen)
    {
        textRenderer->RenderText("You are about to start an encounter.", window_width_px/10, window_height_px*11/20, 0.8, {1,1,1});
        textRenderer->RenderText("Press [ space ] when ready.", window_width_px/10, window_height_px/2, 0.8, {1,1,1});
        glfwSwapBuffers(game->get_window());
        return;
    }
    else if(displayEndScreen){
        textRenderer->RenderText(endScreenMessage, 50, window_height_px /2 , 1, {1,1,1});
        glfwSwapBuffers(game->get_window());
        return;
    }

    // Draw all entities with render requests

    renderer->draw(elapsed_ms_since_last_update, show_pause_menu);
    if (!registry.renderRequests.has(player_profile)) {
        registry.remove_all_components_of(player_profile);
        player_profile = createPlayerProfile();
    }

    if (!registry.renderRequests.has(health_bar)) {
        registry.remove_all_components_of(health_bar);
        health_bar = createHealthBar();
    }
    updateHealthBar(player_entity);

    if (!registry.renderRequests.has(stats_box)) {
        stats_box = registry.create_entity();
        Motion &motion = registry.motions.emplace(stats_box);
        motion.position = {window_width_px - UI_tile_x * 4, UI_tile_y * 20};
        motion.scale = {UI_tile_x * 6, UI_tile_y * 9};
        registry.uiElements.emplace(stats_box);
        registry.renderRequests.insert(stats_box, {
                                       TEXTURE_ASSET_ID::STATS_BOX,
                                       EFFECT_ASSET_ID::TEXTURED,
                                       GEOMETRY_BUFFER_ID::SPRITE,
                                       {},
                                       false
                                   });
    }
    showStats(registry.stats.get(player_entity), game, { window_width_px - UI_tile_x * 6.5, window_height_px - UI_tile_y * 21});

    if(!encounter_system.attacking && !encounter_system.npcAttacking) {
        encounter_system.renderPlayerHealth(textRenderer, current_player_hp, {window_width_px/2.2f, window_height_px/4.0f});
        encounter_system.renderActionButtonText(textRenderer);
        if(registry.renderRequests.has(encounter_attack_box)) {
            registry.renderRequests.remove(encounter_attack_box);
        }
        if(!registry.renderRequests.has(encounter_text_box)) {
            encounter_system.renderEncounterTextBox(encounter_text_box);
        }
        if (!currentMessage.empty()) {
            encounter_system.renderEncounterText(textRenderer, currentMessage, encounter_text_box);
            if(!registry.motions.has(e_press)) {
                Motion& e_press_motion = registry.motions.emplace(e_press);
                Motion& text_box_motion = registry.motions.get(encounter_text_box);
                e_press_motion.position = {
                    text_box_motion.position.x + text_box_motion.scale.x/2.0f - text_box_motion.scale.y/5, 
                    text_box_motion.position.y + text_box_motion.scale.y/2.0f - text_box_motion.scale.y/5
                };
                e_press_motion.scale = {text_box_motion.scale.x/25, text_box_motion.scale.y/5};
                e_press_motion.velocity = {0, 0};

                e_press_animation.frameCount = 3;
                e_press_animation.currentFrame = 0;
                e_press_animation.frameTime = 300.0f;
                e_press_animation.elapsedTime = 0.0f;
                e_press_animation.columns = 3;
                e_press_animation.rows = 1;
                e_press_animation.startRow= 0;
                e_press_animation.startCol = 0;
            }

            e_press_request = {
                TEXTURE_ASSET_ID::E_PRESS,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                e_press_animation,
                true
            };

            if(!registry.renderRequests.has(e_press)) {
                registry.renderRequests.insert(e_press, e_press_request);
            } 
        }
    } else {
        encounter_system.renderPlayerHealth(textRenderer, current_player_hp, {window_width_px/2.2f, window_height_px/5.5f});
        if(encounter_system.attacking) {
            renderTarget();
            renderPath();
        }
        for (const myButton& button : buttons) {
            if (registry.renderRequests.has(button.entity)) {
                registry.renderRequests.remove(button.entity);
            }
        }
        if(registry.renderRequests.has(encounter_text_box)) {
            registry.renderRequests.remove(encounter_text_box);
        }
        if(!registry.renderRequests.has(encounter_attack_box)) {
            encounter_system.renderAttackBox(encounter_attack_box);
        }
    }

    encounter_system.renderNPCName(textRenderer, npc_name, npc_entity);
    encounter_system.renderNPCHealthText(textRenderer, curr_stats, npc_health_bar);
    encounter_system.renderNPCEmotionText(textRenderer, curr_stats, npc_emotion_bar);


    bool is_action = encounter_system.getStatus() == ENCOUNTER_STATUS::PLAYER_ACTION
        || encounter_system.getStatus() == ENCOUNTER_STATUS::NPC_ACTION;
    bool is_win = encounter_system.getStatus() == ENCOUNTER_STATUS::W_KNOCKOUT
        || encounter_system.getStatus() == ENCOUNTER_STATUS::W_INTIMIDATED
        || encounter_system.getStatus() == ENCOUNTER_STATUS::W_CHARMED;
    bool is_lost = encounter_system.getStatus() == ENCOUNTER_STATUS::LOST;

    if (is_action || is_win || is_lost) {
        if (!currentMessage.empty()) {
            encounter_system.renderEncounterText(textRenderer, currentMessage, encounter_text_box);
        }
    }

    glfwSwapBuffers(game->get_window());
}

void EncounterState::handleKnockoutWin(WorldSystem *game) {
    Mix_FadeOutChannel(1, 1000);
    if(displayEndScreen){
        return;
    }
    // reward player with increase to ferocity and minor increase to reputation (relative to npc stats)
    // attack splits reward between ferocity and reputation
    Stats& player_stats = registry.stats.get(player_entity);
    Stats npc_stats = registry.stats.get(npc_entity);
    int ferocityInc = static_cast<int>(npc_stats.ferocity*KNOCKOUT_WIN_FEROCITY_WEIGHT);
    int reputationInc = static_cast<int>(npc_stats.reputation*KNOCKOUT_WIN_REPUTATION_WEIGHT);
    player_stats.ferocity += ferocityInc;
    player_stats.reputation += reputationInc;

    endScreenMessage = "You won! \nFerocity: " + stat_to_string(player_stats.ferocity - ferocityInc) +
    " -> " + stat_to_string(player_stats.ferocity) + "\nReputation : " + stat_to_string(player_stats.reputation - reputationInc) +
    " -> " + stat_to_string(player_stats.reputation)+ "\nPress Space to continue";
    glfwSetInputMode(game->get_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    encounter_system.setStatus(ENCOUNTER_STATUS::DEFAULT);
    displayEndScreen = true;
}

void EncounterState::handleCutenessWin(WorldSystem *game) {
    Mix_FadeOutChannel(1, 1000);
    if(displayEndScreen){
        return;
    }
    // reward player with increase to cuteness (relative to npc stats)
    // Cuteness is rewarded more as it is unconventional.
     Stats& player_stats = registry.stats.get(player_entity);
    Stats npc_stats = registry.stats.get(npc_entity);
    int cutenessInc = static_cast<int>(npc_stats.cuteness*CUTENESS_WIN_WEIGHT);
    player_stats.cuteness += cutenessInc;

     endScreenMessage = "You won! \nCuteness: " + stat_to_string(player_stats.cuteness - cutenessInc) +
    " -> " + stat_to_string(player_stats.cuteness)+ "\nPress Space to continue";
    glfwSetInputMode(game->get_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    encounter_system.setStatus(ENCOUNTER_STATUS::DEFAULT);
    displayEndScreen = true;
}


void EncounterState::handleReputationWin(WorldSystem *game) {
    Mix_FadeOutChannel(1, 1000);
    if(displayEndScreen){
        return;
    }
    // reward player with increase to reputation (relative to npc stats)
    Stats& player_stats = registry.stats.get(player_entity);
    Stats npc_stats = registry.stats.get(npc_entity);
    int reputationInc = static_cast<int>(npc_stats.reputation*REPUTATION_WIN_WEIGHT);
    player_stats.reputation += reputationInc;
    endScreenMessage = "You won! \nReputation: " + stat_to_string(player_stats.reputation - reputationInc) +
    " -> " + stat_to_string(player_stats.reputation) + "\nPress Space to continue";
    glfwSetInputMode(game->get_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    encounter_system.setStatus(ENCOUNTER_STATUS::DEFAULT);
    displayEndScreen = true;
}

void EncounterState::handleLoss(WorldSystem * game) {
    Mix_FadeOutChannel(1, 1000);
    if(displayEndScreen){
        return;
    }
    endScreenMessage = "You lost\nPress space to continue";
    glfwSetInputMode(game->get_window(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    encounter_system.setStatus(ENCOUNTER_STATUS::DEFAULT);
    displayEndScreen = true;
    // std::cout << registry.players.has(player_character) << std::endl;
    // registry.deathTimers.emplace(player_character);
}


bool EncounterState::isGestureMatch(const Gesture& targetGesture){
    if (currentGesturePath.size() != targetGesture.points.size()) {
        renderMisc();
        encounter_system.attacking = false;
        wasMouseClicked = false;
        encounter_system.successfulAttack(player_entity, npc_entity, false);
        currentGesturePath.clear();
        bezierTarget.points.clear();
        return false;
    }
    for(size_t i = 0; i < currentGesturePath.size() - 1; i++){
        // printf("SIZE; %zu \n\n", currentGesturePath.size());
        float currDistance = glm::distance(currentGesturePath[i], currentGesturePath[i+1]);
        float targetDistance = glm::distance(targetGesture.points[i],targetGesture.points[i + 1]);
        float distance = abs( currDistance - targetDistance);
        glm::vec2 currDirection = glm::normalize(currentGesturePath[i + 1] - currentGesturePath[i]);
        glm::vec2 targetDirection = glm::normalize(targetGesture.points[i + 1] - targetGesture.points[i]); 
        float dotProduct = glm::dot(currDirection, targetDirection);
        // printf("DISTANCE : %f Target: %f\n\n", currDistance, targetDistance);
        // printf("Current Direction: (%f, %f)\n", currDirection.x, currDirection.y);
        // printf("Target Direction: (%f, %f)\n", targetDirection.x, targetDirection.y);
        // printf("ANGLE: %f\n\n", dotProduct);
        // if(distance > 80) {
        //     currentGesturePath.clear();
        //     encounter_system.attacking = false;
        //     wasMouseClicked = false;
        //     renderMisc();
        //     encounter_system.successfulAttack(player_entity, npc_entity, false);
        //     return false;
        // } else {
        //     printf("CLOSE ENOUGH DIST \n");
        // }
        if(dotProduct < 0.6) {
            bezierTarget.points.clear();
            currentGesturePath.clear();
            encounter_system.attacking = false;
            wasMouseClicked = false;
            renderMisc();
            encounter_system.successfulAttack(player_entity, npc_entity, false);
            return false;
        } 

    }
    currentGesturePath.clear();
    bezierTarget.points.clear();
    renderMisc();
    encounter_system.successfulAttack(player_entity, npc_entity, true);
    Mix_PlayChannel(2, encounter_system.p_attack_sound, 0);
    encounter_system.attacking = false;
    wasMouseClicked = false;
    return true;
}

void EncounterState::addPoint(vec2 newPoint){

    size_t currentPart = currentGesturePath.size() - 1;

    if(currentPart >= bezierTarget.points.size() - 1) {
        currentPart = bezierTarget.points.size() - 2;
    }
    vec2 start = bezierTarget.points[currentPart];
    vec2 end = bezierTarget.points[currentPart + 1];
    


    float threshold = distance(start, end);

    if((currentGesturePath.empty() || distance(currentGesturePath.back(), newPoint) > threshold)) {
        currentGesturePath.push_back(newPoint);
    }

    

}

void EncounterState::renderMisc(){
    for (const myButton& button : buttons) {
        if (!registry.renderRequests.has(button.entity)) {
            RenderRequest button_render_request = {
            TEXTURE_ASSET_ID::ACTION_BUTTON,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
            {},
            false
        };
        registry.renderRequests.insert(button.entity, button_render_request);
        }
    }
    registry.renderRequests.remove(cat_attack);
    if (!registry.renderRequests.has(npc_emotion_bar)) {
        encounter_system.renderNPCEmotionBar(npc_stats, npc_entity, npc_emotion_bar, npc_emotion_bar_ticker);
    }
    if (!registry.renderRequests.has(npc_emotion_bar_ticker)) {
        encounter_system.renderNPCEmotionText(textRenderer, npc_stats, npc_emotion_bar);
    }
    if (!registry.renderRequests.has(encounter_text_box)) {
        encounter_system.renderEncounterTextBox(encounter_text_box);
    }    
}

void EncounterState::renderTarget(){
    if(currentTarget.points.size() >= 4) {
        for (float t = 0; t <= currentT; t += 0.01f) {
            vec2 pos = cubicBezierInterpolation(t);
            Motion& boxMotion = registry.motions.get(encounter_attack_box);
            textRenderer->RenderText(".", pos.x + boxMotion.scale.x, window_height_px - pos.y, 1, {0, 1, 0});
        }   
    } else {
        for (float t = 0; t <= currentT; t += 0.01f) {
            vec2 pos = quadraticBezierInterpolation(currentTarget.points[0],currentTarget.points[1],
            currentTarget.points[2],t);
            Motion& boxMotion = registry.motions.get(encounter_attack_box);
            textRenderer->RenderText(".", pos.x + boxMotion.scale.x, window_height_px - pos.y, 1, {0, 1, 0});
        }   

    }
    for(size_t i = 0; i < bezierTarget.points.size(); i++){
        textRenderer->RenderText(".", bezierTarget.points[i].x, window_height_px - bezierTarget.points[i].y, 1, {1, 1, 0});
    }

}

void EncounterState::renderPath(){
    if(!currentGesturePath.empty()){
        for(size_t i = 0; i < currentGesturePath.size() - 1; i++){
            textRenderer->RenderText(".", currentGesturePath[i].x, window_height_px - currentGesturePath[i].y, 1, {1, 1, 0});
            vec2 start = currentGesturePath[i];
            vec2 end = currentGesturePath[i+1];
            float distance = glm::distance(start, end);

            int numBetweenPoints = static_cast<int> (distance / 5);       
            for (int j = 1; j <= numBetweenPoints; ++j) {
                float t = static_cast<float>(j) / numBetweenPoints;
                vec2 betweenPoint = start + t * (end - start);
                textRenderer->RenderText(".", betweenPoint.x, window_height_px - betweenPoint.y, 1, {1, 0, 0});

            }  
        }
        textRenderer->RenderText(".", currentGesturePath.back().x, window_height_px - currentGesturePath.back().y, 1, {1, 1, 0});
    }
}

void EncounterState::chooseTarget(){
    std::vector<Gesture*> shapes = {&line, &triangle, &zigzag};
    srand((time(nullptr)));
    int random = rand() % shapes.size();
    Gesture* selected = shapes[random];
    // Gesture* selected = shapes[2];

    float step = 0.2;
    currentTarget = *selected;
    if(selected->points.size() >= 4) {
        for(float t = 0; t <= 1; t += step){
            vec2 pos = cubicBezierInterpolation(t);
            bezierTarget.points.push_back(pos);
        }
    } else {
        for (float t = 0; t<= 1; t+= step){
            vec2 pos = quadraticBezierInterpolation(currentTarget.points[0],currentTarget.points[1],
            currentTarget.points[2],t);
            bezierTarget.points.push_back(pos);
        }
    }
    // currentTarget = *shapes[random];
}

vec2 EncounterState::cubicBezierInterpolation(float t){
    float u = 1 - t;
    float tt = t * t;
    float uu = u * u;
    float ttt = tt * t;
    float uuu = uu * u;

    vec2 p = uuu * currentTarget.points[0];
    p += 3 * uu * t * currentTarget.points[1];
    p += 3 * u * tt * currentTarget.points[2];
    p += ttt * currentTarget.points[3];

    return p;
}

vec2 EncounterState::quadraticBezierInterpolation(const vec2& p0, const vec2& p1, const vec2& p2,float t){
    float u = 1-t;
    float tt = t*t;
    float uu = u * u;

    vec2 p = uu * p0;
    p += 2 * u * t * p1;
    p += tt * p2;

    return p;
}

void EncounterState::updateCurve(float elapsed_ms){
    currentT += step * (elapsed_ms / 1000) * 4;
    if (currentT > 1) {
        currentT = 0;
    }
}

