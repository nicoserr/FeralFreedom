#include "play_state.hpp"
#include "start_state.hpp"
#include "world/world_system.hpp"
#include <iostream>
#include <string>
#include <fstream>

#include "encounter_state.hpp"
#include "game_over_state.hpp"
#include "pause_state.hpp"
#include "core/ecs_registry.hpp"
#include "world/world_init.hpp"
#include "serialization/registry_serializer.hpp"
#include "systems/levels_rooms_system.hpp"
#include "systems/camera_system.hpp"
#include "systems/encounter_system.hpp"

// Game Configuration
const float PICKUP_RADIUS = 100;


PlayState* PlayState::instance() {
    static PlayState instance;
    return &instance;
}

void PlayState::init(WorldSystem* game) {
    // printf("Play state initialized\n");

    renderer = game->get_renderer();
    textRenderer = game->get_text_renderer();
    shadowRenderer = game->get_shadow_renderer();
    inputState = {};
    cameraSystem = CameraSystem();

    std::string state_name = typeid(*this).name();
    std::string file_path = save_path();
    particleSystem = new ParticleSystem(*renderer, TEXTURE_ASSET_ID::SMOKE_PARTICLE, 5, 5000);
    sparkleSystem = new ParticleSystem(*renderer, TEXTURE_ASSET_ID::SMOKE_PARTICLE, 5, 5000);
    keyIconEntity = registry.create_entity();

    if (RegistrySerializer::loadRegistryState(state_name, game)) {
        this->level_manager = game->get_level_manager();

        // registry.renderRequests.clear();
        for (Entity entity: registry.players.entities) {
            if (registry.players.has(entity)) {
                player_character = entity;
                // printf("Player character set to entity %u from loaded state.\n", player_character);
                break;
            }
        }
        for (Entity entity: registry.times.entities) {
            if (registry.times.has(entity)) {
                day_night_entity = entity;
                // printf("Player character set to entity %u from loaded state.\n", player_character);
                break;
            }
        }
        game->get_level_manager()->renderCurrentRoom(player_character);
    } else {
        // printf("State file does not exist. Creating a new %s state.\n", state_name.c_str());
        player_character = createPlayer({0, 0}, 48, 48);
        // printf("Player and patrol created.\n");
        createLevels(game->get_renderer(), game->get_level_manager(), player_character);
        game->get_level_manager()->loadLevel("Test Level", player_character);
        level_manager = game->get_level_manager();
        // std::cout << game->get_level_manager()->levels.size() << std::endl;
        day_night_entity = registry.create_entity();
        registry.times.emplace(day_night_entity);
    }

    // Initialize pathfinding grid for each room
    for (Room * room: game->get_level_manager()->currentLevel->rooms)
    {
        game->get_path_finding_system()->init_grid(
            room->name,
            room->a_star_grid,
            room->non_rendered_entities,
            room->room_width,
            room->room_height
            );
        // game->get_path_finding_system()->print_a_star_grid(room->a_star_grid, room->a_star_grid.size(), room->a_star_grid[0].size());
    }

    bool tutorialNPCExists = false;
    for(Entity npc:registry.npcs.entities) {
        NPC& npc_npc = registry.npcs.get(npc);
        if((npc_npc.isTutorialNPC && npc_npc.isDefeated) || (tutorialFinished && npc_npc.isTutorialNPC)) {
            registry.remove_all_components_of(npc_npc.interactIcon);
            game->get_level_manager()->levels[0]->rooms[1]->remove_entity_from_room(npc);
            registry.remove_all_components_of(npc);
            tutorialNPCExists = true;
            tutorialFinished = true;
        }
    }

    if(!tutorialNPCExists) {
        tutorialFinished = true;
    }

    chase_active = false;
}

void PlayState::pause() {
    //printf("Play state paused\n");
}

void PlayState::resume() {
    //printf("Play state resumed\n");
}

void PlayState::cleanup(WorldSystem *game) {
    std::string state_name = typeid(*this).name();
    // Save the current state before cleaning up
    // printf("Saving registry state for %s...\n", stateName.c_str());

    // registry.list_all_components();
    // printf("Cleaning up play state.\n");

    RegistrySerializer::cleanupStateFile(state_name);
    RegistrySerializer::saveRegistryState(state_name, game);

    // printf("Registry state before cleanup:\n");
    // printf("Number of entities in registry: %zu\n", registry.get_entities().size());

    registry.get_entities().clear(); // we need this or else it doesn't clean the entire entity list
    registry.clear_all_components();
    // if i do this i get // Assertion failed: (!(check_for_duplicates && has(e)) && "Entity already contained in ECS registry"), function insert, file ecs.hpp, line 62.
    // game->get_level_manager()->cleanUpAllRooms();
    // with this i get // Assertion failed: (has(e) && "Entity not contained in ECS registry"), function get, file ecs.hpp, line 93.
    game->get_level_manager()->cleanUpCurrentRoom();

    player_character = Entity();
    keyIconEntity = Entity();

    // printf("Registry state after cleanup:\n");
    // printf("Number of entities in registry: %zu\n", registry.get_entities().size());
}

void PlayState::reset(WorldSystem *game) {
    // printf("Cleaning up PlayState files...\n");
    std::string state_name = typeid(*this).name();

    // Remove the existing JSON file to ensure a fresh save each time
    RegistrySerializer::cleanupStateFile(state_name);

    // Restart levels
    game->get_level_manager()->reset();
}

void PlayState::handle_mouse_input(WorldSystem *game, double xpos, double ypos) {
    // Check if the mouse click is inside the show_stats button
    if (is_click_inside_button({xpos, ypos}, showStatsButton, *game) || is_click_inside_button({xpos, ypos}, hideStatsButton, *game)) {
        show_stats = !show_stats; // Toggle the display of stats
    }
}

void PlayState::handle_key_input(WorldSystem* game, int key, int scancode, int action, int mods) {
    if (action == GLFW_RELEASE && key == GLFW_KEY_ESCAPE && !registry.renderRequests.has(dialogueBox)) {
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

    //Toggle FPS counter
    if (action == GLFW_PRESS && key == GLFW_KEY_F) {
        show_fps = !show_fps;
    }

    //Interact button pressed
    if (action == GLFW_PRESS && key == GLFW_KEY_E) {
        inputState.up = false;
        inputState.down = false;
        inputState.left = false;
        inputState.right = false;

        if(shouldShowSelect) {
            if(selectArrowPos == 2) {
                currentMessage = "";
                shouldShowSelect = false;
            } else {
                //begin encounter
                currentMessage = "";
                shouldShowSelect = false;
                NPC& encounter_npc = registry.npcs.get(dialogueNPC);
                encounter_npc.start_encounter = true;
            }
        } else if(shouldShowDialogue || (registry.renderRequests.has(dialogueBox) && !shouldPickUpItem)) {
            //Render dialogue box
            NPC& dialogue_npc = registry.npcs.get(dialogueNPC);
            KeyInventory& keyInventory = registry.keyInventory.get(player_character);
            if(dialogue_npc.dialogue.empty() && registry.doors.has(dialogueNPC) && keyInventory.keys.size() < registry.doors.get(dialogueNPC).required_keys){
                std::string message = std::to_string(registry.doors.get(dialogueNPC).required_keys - registry.keyInventory.get(player_character).keys.size());
                dialogue_npc.dialogue.push("NEED " + message + " KEYS TO ENTER");
            } else if (dialogue_npc.dialogue.empty() && registry.doors.has(dialogueNPC)){
                dialogue_npc.dialogue.push("YOU USE THE 3 KEYS TO OPEN THE DOOR");
                dialogue_npc.dialogue.push("YOU ARE ABOUT TO ENTER A BOSS ROOM");
                dialogue_npc.dialogue.push("BE PREPARED");
            }
            if(currentDialogue.empty() && currentMessage == "") {
                currentDialogue = dialogue_npc.dialogue;
            }

            if(!currentDialogue.empty()) {
                currentMessage = currentDialogue.front();
                currentDialogue.pop();
                if(registry.doors.has(dialogueNPC)){
                    dialogue_npc.dialogue.pop();
                }

            } else {
                if(registry.doors.has(dialogueNPC)){
                    dialogue_npc.dialogue.pop();
                }
                currentMessage = "";
            }

            if(currentMessage == "" && dialogue_npc.isInteractable) {
                currentMessage = "Start Encounter?";
                shouldShowSelect = true;
            }
        } else if(shouldPickUpItem) {
            if(currentMessage == "") {
                itemCollection(player_character);
                shouldRenderEPress = false;
            } else {
                currentMessage = "";
                shouldPickUpItem = false;
            }
        }
    }

    if(!registry.renderRequests.has(dialogueBox)) {
        if ((action == GLFW_PRESS || action == GLFW_RELEASE)) {
            bool isPressed = (action == GLFW_PRESS);
            if (key == GLFW_KEY_W) inputState.up = isPressed;
            if (key == GLFW_KEY_S) inputState.down = isPressed;
            if (key == GLFW_KEY_A) inputState.left = isPressed;
            if (key == GLFW_KEY_D) inputState.right = isPressed;
        }
    }

    if(shouldShowSelect) {
        if((action == GLFW_PRESS)) {
            if(key == GLFW_KEY_W || key == GLFW_KEY_S || key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
                if(selectArrowPos == 1) {
                    selectArrowPos = 2;
                } else {
                    selectArrowPos = 1;
                }
            }
        }
    }

    // TODO: make encounter begin when colliding with pre-existing NPC instead of a temp one.
    if (action == GLFW_RELEASE && key == GLFW_KEY_0) {
        // printf("[0] PRESSED in start state: switching to play screen\n");
        Entity temp = registry.create_entity();
        Stats &temp_stats = registry.stats.emplace(temp);
        Motion &temp_motion = registry.motions.emplace(temp);
        NPC &temp_npc = registry.npcs.emplace(temp);
        temp_npc.encounter_texture_id = (int) TEXTURE_ASSET_ID::TEMP_NPC;
        temp_npc.name = "Tutorial Dummy";
        temp_npc.attackDuration = 10.0f;

        Stats &player_stats = registry.stats.get(player_character);
        player_stats.intelligence = 9;

        // registry.npcs.emplace(temp_npc);
        temp_stats.maxHp = 20;
        temp_stats.currentHp = 1;
        temp_stats.cuteness = 1;
        temp_stats.reputation = 1;
        temp_stats.ferocity = 1;
        beginEncounter(game, player_character, temp);
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        game->push_state(PauseState::instance()); // Pauses and overlays the PauseState
    }

    // if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    //     // printf("SPACE PRESSED in play state: switching to start screen\n");
    //     shadowRenderer->clearShadows();
    //     game->change_state(StartState::instance());
    // }
}

void PlayState::updatePlayerAnimation() {
    Motion& playerMotion = registry.motions.get(player_character);
    RenderRequest &request = registry.renderRequests.get(player_character);

    if (playerMotion.velocity.x > 0) {
        if (playerMotion.velocity.y > 0) {
            renderer->setAnimation(request, AnimationState::MOVING_UP_RIGHT, renderer->catAnimationMap);
        } else if (playerMotion.velocity.y < 0) {
            renderer->setAnimation(request, AnimationState::MOVING_DOWN_RIGHT, renderer->catAnimationMap);
        } else {
            renderer->setAnimation(request, AnimationState::MOVING_RIGHT, renderer->catAnimationMap);
        }
    } else if (playerMotion.velocity.x < 0) {
        if (playerMotion.velocity.y > 0) {
            renderer->setAnimation(request, AnimationState::MOVING_UP_LEFT, renderer->catAnimationMap);
        } else if (playerMotion.velocity.y < 0) {
            renderer->setAnimation(request, AnimationState::MOVING_DOWN_LEFT, renderer->catAnimationMap);
        } else {
            renderer->setAnimation(request, AnimationState::MOVING_LEFT, renderer->catAnimationMap);
        }
    } else if (playerMotion.velocity.y > 0) {
        renderer->setAnimation(request, AnimationState::MOVING_UP, renderer->catAnimationMap);
    } else if (playerMotion.velocity.y < 0) {
        renderer->setAnimation(request, AnimationState::MOVING_DOWN, renderer->catAnimationMap);
    } else {
        renderer->setAnimation(request, AnimationState::IDLE, renderer->catAnimationMap);
    }
}

void PlayState::updateDogAnimation(Entity dog) {
    Motion& motion = registry.motions.get(dog);
    RenderRequest& renderRequest = registry.renderRequests.get(dog);

    if (motion.velocity.x == 0 && motion.velocity.y == 0) {
        renderer->setAnimation(renderRequest, AnimationState::LAYING_DOWN, renderer->dogAnimationMap);
        renderRequest.flip_horizontal = false;
        return;
    }

    // Determine the dominant direction (x or y axis)
    if (std::abs(motion.velocity.x) > std::abs(motion.velocity.y)) {
        if (motion.velocity.x > 0) {
            // Moving right
            renderer->setAnimation(renderRequest, AnimationState::MOVING_LEFT, renderer->dogAnimationMap);
            renderRequest.flip_horizontal = true; // Flip for right-facing
        } else {
            // Moving left
            renderer->setAnimation(renderRequest, AnimationState::MOVING_LEFT, renderer->dogAnimationMap);
            renderRequest.flip_horizontal = false; // No flip for left-facing
        }
    } else {
        // Vertical movement uses left-facing sprite
        renderer->setAnimation(renderRequest, AnimationState::MOVING_LEFT, renderer->dogAnimationMap);
        renderRequest.flip_horizontal = false;
    }
}


void PlayState::updatePatrolAnimation(Entity patrol) {
    Motion& motion = registry.motions.get(patrol);
    RenderRequest& renderRequest = registry.renderRequests.get(patrol);

    if (motion.velocity.x == 0 && motion.velocity.y == 0) {
        renderer->setAnimation(renderRequest, AnimationState::IDLE, renderer->npcAnimationMap);
        return;
    }

    // Determine the dominant direction (x or y axis) because we don't have diagonal sprites for patrol
    if (std::abs(motion.velocity.x) > std::abs(motion.velocity.y)) {
        // Horizontal movement
        if (motion.velocity.x > 0) {
            renderer->setAnimation(renderRequest, AnimationState::MOVING_RIGHT, renderer->npcAnimationMap);
        } else {
            renderer->setAnimation(renderRequest, AnimationState::MOVING_LEFT, renderer->npcAnimationMap);
        }
    } else {
        // Vertical movement
        if (motion.velocity.y > 0) {
            renderer->setAnimation(renderRequest, AnimationState::MOVING_DOWN, renderer->npcAnimationMap);
        } else {
            renderer->setAnimation(renderRequest, AnimationState::MOVING_UP, renderer->npcAnimationMap);
        }
    }
}

void PlayState::update(WorldSystem *game, float elapsed_ms) {
    if (show_pause_menu || show_exit_menu) return; // Don't update if paused or quitting

    Player &player = registry.players.get(player_character);
    Stats &playerStats = registry.stats.get(player_character);

    if (playerStats.currentHp <= 0 && !player.lost_life) {
        player.lives--;
        player.lost_life = true;
        player.lost_life_timer_ms = 4000.f;
        playerStats.currentHp = playerStats.maxHp;
        printf("Player lost a life. Remaining lives: %d\n", player.lives);
        // registry.deathTimers.emplace(player_character);

        if (player.lives <= 0) {
            player.lost_life = false;
            game_over = true;
        }
    }

    // for (Entity npc_entity : registry.npcs.entities)
    // {
    //     NPC& npc = registry.npcs.get(npc_entity);
    //     // Handle NPC encounter resolution. If we are here, then the player beat the encounter.
    //     if (npc.start_encounter)
    //     {
    //         // npc.blocked_door
    //         if (npc.blocking_door) {
    //             Entity blocked_door = registry.get_entity_by_id(npc.blocked_door);
    //             if (blocked_door.getId() < UINT_MAX)
    //                 registry.doors.get(blocked_door).is_open = true;
    //         }
    //         beginEncounter(game, player_character, npc_entity);
    //         return;
    //     }
    // }
    Motion &playerMotion = registry.motions.get(player_character);

    particleSystem->update(elapsed_ms);
    sparkleSystem->update(elapsed_ms);

    // Update game logic (e.g., player movement, collisions)
    //Finds magnitude of speed so that diagonal movement will be the same as others.
    playerControls.x = 0;
    playerControls.y = 0;

    // Calculate movement based on current input state
    if (inputState.up) playerControls.y -= PLAYER_SPEED;
    if (inputState.down) playerControls.y += PLAYER_SPEED;
    if (inputState.left) playerControls.x -= PLAYER_SPEED;
    if (inputState.right) playerControls.x += PLAYER_SPEED;

    // Normalize to prevent faster diagonal movement
    float mag = sqrt(playerControls.x * playerControls.x + playerControls.y * playerControls.y);
    if (mag > 0) {
        playerMotion.velocity = {(playerControls.x / mag) * PLAYER_SPEED, (playerControls.y / mag) * PLAYER_SPEED};
    } else {
        playerMotion.velocity = {0, 0};
    }

    updatePlayerAnimation();
    if (game->get_level_manager()->currentLevel->currentRoom->name == "map room") {
        vec2 mapSize = {4096, 4096};
        vec2 windowSize = {window_width_px, window_height_px};
        cameraSystem.update(player_character, mapSize, windowSize);
    } else if (game->get_level_manager()->currentLevel->currentRoom->name == "office room" ||
               game->get_level_manager()->currentLevel->currentRoom->name == "vet room") {
        vec2 mapSize = {1024, 1024};
        vec2 windowSize = {window_width_px, window_height_px};
        cameraSystem.update(player_character, mapSize, windowSize);
    } else {
        renderer->cameraTransform = mat3(1.0f);
    }
    //update entities speed
    float stepSeconds = elapsed_ms / 1000.f;

    for (Entity entity: registry.motions.entities) {
        if (game->get_level_manager()->currentLevel->currentRoom->isEntityInRoom(entity)) {
            Motion &motion = registry.motions.get(entity);
            motion.position += motion.speedMod * stepSeconds * motion.velocity;
            motion.speedMod = 1.0f;
        }
    }

    ////////////////////////////////////////
    //Run collision system
    CollisionSystem cs = *game->get_collision_system();
    cs.step(game->get_level_manager());

    Room* currentRoom = game->get_level_manager()->currentLevel->currentRoom;

    // Update the timer of Random walkers
    for (Entity entity : registry.randomWalkers.entities) {
        if (currentRoom->isEntityInRoom(entity)) {
            RandomWalker& rnd_walker = registry.randomWalkers.get(entity);
            rnd_walker.sec_since_turn += stepSeconds;
        }
    }

    ///////////////////////////////////////////////////
    // NPC/ENCOUNTER MANAGEMENT
    for(Entity entity: currentRoom->rendered_entities) {
        //For NPC Encounters (instead of doing two seperate for loops)
        if(registry.npcs.has(entity)){
            NPC& npc = registry.npcs.get(entity);
            if (npc.name == "dog" && npc.isInteractable) {
                updateDogAnimation(entity);
            }
            // Handle NPC encounter resolution. If we are here, then the player beat the encounter.
            if (npc.isInteractable && npc.start_encounter)
            {
                // npc.blocked_door
                if (npc.blocking_door) {
                    Entity blocked_door = registry.get_entity_by_id(npc.blocked_door);
                    if (blocked_door.getId() < UINT_MAX)
                        registry.doors.get(blocked_door).is_open = true;
                }
                beginEncounter(game, player_character, entity);
                return;
            }
            if(npc.to_remove){
                registry.remove_all_components_of(npc.interactIcon);

                npc.start_encounter = false;
                npc.to_remove = false;
                npc.isFadingOut = true;
                npc.fadeOutTimer = 2.f;
                npc.isDefeated = true;
                glm::vec2 npcPosition = registry.motions.get(entity).position;
                glm::vec4 smokeColor = glm::vec4(1, 1, 1, 1.0f);
                particleSystem->setCurrentRoom(currentRoom);
                particleSystem->setInitialParameters({npcPosition.x, npcPosition.y}, {0, -10}, smokeColor);
                particleSystem->emit({npcPosition.x, npcPosition.y}, {0, -10}, smokeColor);
                if(npc.dropKey){
                    createKey(npcPosition);
                }
                game->get_level_manager()->renderCurrentRoom(player_character);
            }
            if(npc.isFadingOut){
                RenderRequest& npcRender = registry.renderRequests.get(entity);
                npc.fadeOutTimer -= elapsed_ms / 1000.f;
                float alpha = std::max(0.0f, npc.fadeOutTimer / 1.0f);
                npcRender.alpha = alpha;
                // std::cout << "Alpha value: " << npcRender.alpha << std::endl;
                if(npc.fadeOutTimer <= 0 && npcRender.alpha <= 0) {
                    game->get_level_manager()->currentLevel->currentRoom->remove_entity_from_room(entity);
                    registry.remove_all_components_of(entity);
                }
            }
        }

        // PATROL MANAGEMENT
        if(registry.patrols.has(entity)) {
            //std::cout << "Has patrol" << std::endl;
            Patrol &patrol = registry.patrols.get(entity);
            updatePatrolAnimation(entity);

            // TODO: Add SetMotion patrol here
            if (registry.setMotions.has(entity))
            {
                SetMotion& sm = registry.setMotions.get(entity);
                Motion& patrol_motion = registry.motions.get(entity);

                patrol_motion.velocity.y = 0;

                if (patrol_motion.velocity.x == 0)
                {
                    patrol_motion.velocity.x = patrol_motion.speed;
                }

                if ((sm.timer <= 0))
                {
                    if (patrol_motion.position.x >= 700)
                    {
                        patrol_motion.velocity.x = -1* patrol_motion.speed;
                        patrol_motion.angle += M_PI/2;
                    }
                    else if (patrol_motion.position.x <= 300)
                    {
                        patrol_motion.velocity.x = patrol_motion.speed;
                        patrol_motion.angle += M_PI/2;
                    }

                    sm.timer = 1000;

                } else
                {
                    sm.timer -= 1;
                }
                //updatePatrolRendering(patrol_motion, entity);
                continue;
            }

            Motion& motion = registry.motions.get(entity);
            Motion& lightMotion = registry.motions.get(patrol.light);
            lightMotion.position = motion.position;
            lightMotion.angle = motion.angle + M_PI / 2.f;
            lightMotion.scale = { 64.f * 3.f,64.f * 3.f }; //magic number
            lightMotion.position.x += cos(motion.angle) * 135.f; //more magic numbers, horaay!
            lightMotion.position.y += sin(motion.angle) * 135.f;


            //////////////////////////////
            // CHASE LOGIC
            if (patrol.player_seen) {
                chase_active = true;
                if (!registry.chasers.has(entity)) {
                    Chaser& chaser = registry.chasers.emplace(entity);
                    chaser.target_pos = registry.motions.get(player_character).position;
                    chaser.counter_ms = CHASE_DURATION;
                    // printf("Added chaser component to entity %u\n", entity);
                }
                else {
                    Chaser& chaser = registry.chasers.get(entity);
                    chaser.counter_ms = CHASE_DURATION;
                }
                if (registry.convergers.has(entity)) {
                    //if you are a chaser, don't be a converger
                    registry.convergers.remove(entity);
                }
            }
            else if (chase_active) { //chase active but this patrol specifically didn't see the player
                //make into a converger if not already
                if (registry.chasers.has(entity))
                {
                    if (registry.chasers.get(entity).counter_ms <= 0)
                        registry.chasers.remove(entity);
                }
                else if (!registry.convergers.has(entity))
                {
                    Converger& converger = registry.convergers.emplace(entity);
                    converger.target_pos = registry.motions.get(player_character).position;
                    converger.counter_ms = CONVERGER_UPDATE_TIME;
                }
                else
                {
                    Converger& converger = registry.convergers.get(entity);
                }
            }
        }
        if(registry.key.has(entity)){
            Key& key = registry.key.get(entity);
            RenderRequest& keyRender = registry.renderRequests.get(entity);

            if(key.fadeTimer > 0.f) {
                key.fadeTimer -= elapsed_ms / 1000.f;
                keyRender.alpha = std::min(1.0f, 1.0f - (key.fadeTimer / 2.0f));
            }
        }
    }


    // Handle ending chases. Pre-emptively set chase_disengaged to true if chase is active.
    // Re-set to false if chase should be active.
    // Note that we can't set chase_active to false at start of update()
    // because then if a patrol sees the player, patrols "ahead" of this one will not become convergers
    if (chase_active) {
        chase_disengaged = true;
    }
    chase_active = false;

    for(Entity& entity: currentRoom->rendered_entities)
    {
        if (registry.chasers.has(entity) && registry.chasers.get(entity).counter_ms > 0)
        {
            chase_active = true;
            chase_disengaged = false;
        }
        else if (registry.patrols.has(entity) && registry.patrols.get(entity).player_seen)
        {
            chase_active = true;
            chase_disengaged = false;
        }
    }

    if (chase_disengaged) {
        resetPatrolMovement();
    }

    if (chase_active) {
        // Pathfinding is necessary
        game->get_path_finding_system()->step(level_manager->currentLevel->currentRoom->a_star_grid,
            level_manager->currentLevel->currentRoom->non_rendered_entities);

        updateChasers(elapsed_ms);
        updateConvergers(elapsed_ms);
        if (!registry.visualEffects.has(player_character)) {
            // add redness effect if chase is active
            game->get_visual_effects_system()->addEffect(player_character, "redness", 500, 0);
        }
    }
    else {
        updateRandomWalkers();
    }
    GLuint shaderProgram = game->get_renderer()->getEffectProgram(EFFECT_ASSET_ID::BLACK);
    game->get_visual_effects_system()->update(elapsed_ms, shaderProgram);

    // update the rendering of patrol's sprite
    // for (Entity entity : registry.patrols.entities) {
    //     Motion& patrolMotion = registry.motions.get(entity);
    //     updatePatrolRendering(patrolMotion, entity);
    // }
    //Updates pickup cooldown on items to avoid calculations happening multiple times in same frame
    for (Entity item : registry.equippableItems.entities) {
        EquippableItem& equippable = registry.equippableItems.get(item);
        if (equippable.pickupCooldown > 0) {
            equippable.pickupCooldown -= elapsed_ms / 1000.f;
            if (equippable.pickupCooldown < 0) {
                equippable.pickupCooldown = 0;
            }
        }
    }

    // Check if player has recently lost a life and reset the flag after a short delay
    if (player.lost_life) {
        player.lost_life_timer_ms -= elapsed_ms;
        if (player.lost_life_timer_ms <= 0) {
            player.lost_life = false;
        }
    }

    if(shouldRenderEPress) {
        Motion& playerMotion = registry.motions.get(player_character);
        if(!registry.motions.has(e_press)) {
            Motion& eMotion = registry.motions.emplace(e_press);

            eMotion.scale = playerMotion.scale/2.5f;
            eMotion.velocity = {0, 0};
            eMotion.position = {playerMotion.position.x, playerMotion.position.y - playerMotion.scale.y/1.7f};
            eMotion.z = 1;
        } else {
            Motion& eMotion = registry.motions.get(e_press);
            eMotion.position = {playerMotion.position.x, playerMotion.position.y - playerMotion.scale.y/1.7f};
        }
    }

    if(shouldShowSelect) {
        if(registry.motions.has(selectArrow)) {
            Motion& arrow_motion = registry.motions.get(selectArrow);

            if(selectArrowPos == 1) {
                arrow_motion.position.y = window_height_px - window_height_px/3.1f;
            } else {
                arrow_motion.position.y = window_height_px - window_height_px/3.8f;
            }
        }
    }

    checkInNPCRange(game);

    Time& clockTime = registry.times.get(day_night_entity);
    clockTime.time += elapsed_ms;

    if(game->get_level_manager()->currentLevel->currentRoom->name == "map room") {
        //Day-Night logic
        //Assumes 3 real life seconds is 10 in game minutes,
        //Sunrise at 7:00 and Sunset at 19:00 (for simplicity of linear interpolation)
        int time = clockTime.time / 300.f;
        time %= 24 * 60;
        float lerpTime = (time-7.f * 60.f)/(12.f * 60.f);
        light_amount = 1.0 - max(min(sin(lerpTime * M_PI) + 0.5, 1.0), 0.0);
        //game->get_visual_effects_system()->addEffect(day_night_entity, "vignette", 1000.0f/60.0, vignette_amount);

        overlay_color = {0,0,0,0};
        if (time >= 18.f * 60.f && time <= 20.f * 60.f) {
            lerpTime = (time-18.f * 60.f)/(2.f * 60.f);
            overlay_color = {1.f, abs(1.f-lerpTime*2.f), max(abs(1.f-lerpTime*4.f),0.f), (lerpTime - lerpTime*lerpTime) * 0.66f};
        }
    } else {
        overlay_color = {0,0,0,0};
        light_amount = 0.0f;
        //game->get_visual_effects_system()->addEffect(day_night_entity, "vignette", 1000.0f/60.0f, 0.0f);
    }
}

void PlayState::draw(WorldSystem* game, float elapsed_ms_since_last_update) {
    RenderSystem* renderer = game->get_renderer();
    mat3 projection = renderer->createProjectionMatrix();

    if(game->get_level_manager()->currentLevel->currentRoom->name == "map room" ||
    game->get_level_manager()->currentLevel->currentRoom->name == "office room" ||
    game->get_level_manager()->currentLevel->currentRoom->name == "vet room") {
        mat3 cameraTransform = cameraSystem.getCameraTransform();
        vec2 cameraCenter = {cameraTransform[2].x, cameraTransform[2].y};
        mat3 cameraProjection = renderer->createCameraProjection(cameraCenter);

        shadowRenderer->RenderShadows(level_manager, cameraTransform);

        renderer->drawTransform(elapsed_ms_since_last_update, cameraTransform);
        renderer->draw(elapsed_ms_since_last_update, show_pause_menu, overlay_color, light_amount);
    }
    else {
        // Draw all entities with render requests
        shadowRenderer->RenderShadows(level_manager, mat3(1.0f));
        renderer->draw(elapsed_ms_since_last_update, show_pause_menu, overlay_color, light_amount);
    }

    //Draws the in-game time based on variable "clockTime"
    //Assumes that 3 real life seconds translate to 10 in game minute
    Time& clockTime = registry.times.get(day_night_entity);
    int minutes = clockTime.time / 300.0f;
    int hours = (minutes/60) % 24;
    minutes %= 60;
    std::string hoursString = std::to_string(hours);
    std::string minutesString = std::to_string(minutes);
    if (hours < 10) {
        hoursString = "0" + hoursString;
    }
    if (minutes < 10) {
        minutesString = "0" + minutesString;
    }
    textRenderer->RenderText(hoursString + ":" + minutesString, 40, window_height_px - 40,
                             defaultTextScale * 2, {1, 1, 1});

    if (show_fps) {
        fpsTimer += elapsed_ms_since_last_update / 1000;
        fpsCount++;
        textRenderer->RenderText("FPS: " + std::to_string(static_cast<int>(fpsCount / fpsTimer)), 40, 40,
                                 defaultTextScale * 2, {1, 1, 1});
        if (fpsTimer > 1) {
            fpsCount = 0;
            fpsTimer = 0;
        }
    }
    if (show_exit_menu) {
        promptSaveOnExit(game);
    }

    drawUIElements(game, player_character); // profile, stats, health bar, hearts
    // creates if it doesn't exist, and updates
    //renderHearts(registry.players.get(player_character).lives);
    drawEquipped(player_character);

    projection = renderer->createProjectionMatrix();
    particleSystem->draw(projection, cameraSystem.getCameraTransform(), level_manager->currentLevel->currentRoom);
    sparkleSystem->draw(projection, cameraSystem.getCameraTransform(), level_manager->currentLevel->currentRoom);

    drawKey();
    renderPickupText(game);
    renderKeyText(game);
    renderConsumeText(game);
    renderDialogueText(game);
    renderSelectBox(game);
    renderSelectArrow(game);

    if(shouldRenderEPress) {
        if(!registry.motions.has(e_press)) {
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
    } else {
        if(registry.renderRequests.has(e_press)) {
            registry.remove_all_components_of(e_press);
        }
    }


    glfwSwapBuffers(game->get_window());
}

void PlayState::beginEncounter(WorldSystem* game, Entity player_entity, Entity npc_entity) {
    NPC& npc = registry.npcs.get(npc_entity);
    Stats npc_stats = registry.stats.get(npc_entity);

    // // Remove NPC from registry and level_manager before encounter
    // game->get_level_manager()->currentLevel->currentRoom->remove_entity_from_room(npc_entity);
    // registry.remove_all_components_of(npc_entity);

    npc.isInteractable = false;
    npc.to_remove = true;
    bool isTutorial = registry.npcs.get(npc_entity).isTutorialNPC;
    shadowRenderer->clearShadows();
    game->change_state(EncounterState::instance(registry.stats.get(player_entity),
        npc_stats, registry.get_entity_index(player_entity), npc.encounter_texture_id, npc.name, isTutorial));

}



void PlayState::updateChasers(float elapsed_ms) const
{
    for (Entity e : registry.chasers.entities) {
        Chaser& chaser = registry.chasers.get(e);
        Motion& motion = registry.motions.get(e);
        vec2 direction = chaser.target_pos - motion.position;

        Motion& player_motion = registry.motions.get(player_character);

        if (motion.position == player_motion.position) {
            motion.velocity = vec2(0, 0);
        }
        else {
            // Normalize direction and set velocity
            vec2 normalized_direction = normalize(direction);
            motion.velocity = normalized_direction * motion.speed;
            motion.angle = atan2(normalized_direction.y, normalized_direction.x);
        }
        chaser.target_pos = registry.motions.get(player_character).position;
        chaser.counter_ms -= elapsed_ms;
    }
}

void PlayState::updateConvergers(float elapsed_ms) {
    for (Entity& e : registry.convergers.entities) {
        Converger& converger = registry.convergers.get(e);
        Motion& motion = registry.motions.get(e);

        if (converger.path_to_target.empty()) {
            // Converger will stop moving completely when at target position.
            motion.velocity = vec2(0, 0);

            // Reset converger to find path again
            converger.path_found = false;
            converger.counter_ms = CONVERGER_UPDATE_TIME;
            return;
        }

        vec2 nextPos = converger.path_to_target.front();

        // Get the target position from the next node. TODO: Get cell_size from pathfinding.
        float cell_size = 5.f;
        vec2 direction = nextPos - motion.position;

        if (length(direction) <= cell_size) {
            // Converger has arrived at the node, move to the next pos
            converger.path_to_target.erase(converger.path_to_target.begin());
        }
        else {
            // Normalize direction and set velocity
            vec2 normalized_direction = normalize(direction);
            motion.velocity = normalized_direction * motion.speed;
            motion.angle = atan2(normalized_direction.y, normalized_direction.x);
        }
        converger.target_pos = registry.motions.get(player_character).position;

        converger.counter_ms -= elapsed_ms;

        // Re-find path
        if (converger.counter_ms <= 0)
        {
            converger.counter_ms = CONVERGER_UPDATE_TIME;
            converger.path_found = false;
            converger.path_to_target.clear();
        }
    }
}


void PlayState::updateRandomWalkers() {
    for (Entity e : registry.randomWalkers.entities) {
        RandomWalker& rnd_walker = registry.randomWalkers.get(e);

        if (rnd_walker.sec_since_turn > rnd_walker.sec_to_turn) {
            // Change direction
            Motion& rnd_walker_motion = registry.motions.get(e);
            change_direction(rnd_walker_motion);

            // Reset sec until next direction change
            rnd_walker.sec_since_turn = 0;
            rnd_walker.sec_to_turn = random_step(rng);
        }
    }
}

void PlayState::resetPatrolMovement() {
    for (Entity& chaser : registry.chasers.entities) {
        registry.chasers.remove(chaser);
    }

    for (Entity converger : registry.convergers.entities) {
        registry.convergers.remove(converger);
    }
}

void PlayState::updatePatrolRendering(Motion& patrol_motion, Entity patrol) {

    RenderRequest& request = registry.renderRequests.get(patrol);

    if (registry.setMotions.has(patrol))
    {
        if (patrol_motion.velocity.x < 0)
        {
            request.used_texture = TEXTURE_ASSET_ID::SET_MOTION_LEFT;
        } else
        {
            request.used_texture = TEXTURE_ASSET_ID::SET_MOTION_RIGHT;
        }
        return;
    }

    if (patrol_motion.velocity.x < 0 && patrol_motion.velocity.y < 0) {
        if (request.used_texture != TEXTURE_ASSET_ID::PATROL_LEFT) {
            request.used_texture = TEXTURE_ASSET_ID::PATROL_UP;
        }
    }

    if (patrol_motion.velocity.x > 0 && patrol_motion.velocity.y > 0) {
        if (request.used_texture != TEXTURE_ASSET_ID::PATROL_RIGHT) {
            request.used_texture = TEXTURE_ASSET_ID::PATROL_DOWN;
        }
    }

    if (patrol_motion.velocity.x < 0 && patrol_motion.velocity.y > 0) {
        if (request.used_texture != TEXTURE_ASSET_ID::PATROL_LEFT) {
            request.used_texture = TEXTURE_ASSET_ID::PATROL_DOWN;
        }
    }

    if (patrol_motion.velocity.x > 0 && patrol_motion.velocity.y < 0) {
        if (request.used_texture != TEXTURE_ASSET_ID::PATROL_RIGHT) {
            request.used_texture = TEXTURE_ASSET_ID::PATROL_UP;
        }
    }
    else if (patrol_motion.velocity.x < 0 && patrol_motion.velocity.y == 0) {
        request.used_texture = TEXTURE_ASSET_ID::PATROL_LEFT;
    }
    else if (patrol_motion.velocity.x > 0 && patrol_motion.velocity.y == 0) {
        request.used_texture = TEXTURE_ASSET_ID::PATROL_RIGHT;
    }
    else if (patrol_motion.velocity.y < 0 && patrol_motion.velocity.x == 0) {
        request.used_texture = TEXTURE_ASSET_ID::PATROL_UP;
    }
    else if (patrol_motion.velocity.y > 0 && patrol_motion.velocity.x == 0) {
        request.used_texture = TEXTURE_ASSET_ID::PATROL_DOWN;
    }
}

void PlayState::change_direction(Motion& motion) {
    float angle = angles[random_angle(rng)];

    while (angle == motion.angle) {
        angle = angles[random_angle(rng)];
    }

    motion.angle = angle;
    motion.velocity.x = motion.speed * cos(angle);
    motion.velocity.y = motion.speed * sin(angle);
}

void PlayState::itemCollection(Entity player) {
    if (!registry.inventory.has(player) || !registry.keyInventory.has(player)) {
        return;
    }
    Motion& playerMotion = registry.motions.get(player);
    Inventory& inventory = registry.inventory.get(player);
    KeyInventory& keyInventory = registry.keyInventory.get(player);
    Stats& playerStats = registry.stats.get(player);
    Room* currentRoom = level_manager->currentLevel->currentRoom;
    std::vector<Entity> toRemove;

    for (Entity& item : registry.consumableItems.entities) {
        if(!currentRoom->isEntityInRoom(item)){
            continue;
        }
        Motion& itemMotion = registry.motions.get(item);
        float distance = length(itemMotion.position - playerMotion.position);
        if (distance < PICKUP_RADIUS) {
            currentMessage = "";
            ConsumableItem& consumable = registry.consumableItems.get(item);
            vec4 sparkleColor;
            if(consumable.statModifiers.currentHp < 0) {
                sparkleColor = {0.8f, 0.4f, 0.2f, 1.0f};
            } else {
                sparkleColor = {0.4f, 0.8f, 0.2f, 1.0f};
            }
            
            sparkleSystem->setCurrentRoom(currentRoom);
            sparkleSystem->setInitialParameters(itemMotion.position, {0, -10}, sparkleColor);
            sparkleSystem->emit(itemMotion.position, {0, -10}, sparkleColor);
            //printf("PLAYER STAT BEFORE: \n");
            //printStats(playerStats);
            // printf("ITEM STATS: \n");
            // printStats(consumable.statModifiers);
            int oldHP = playerStats.currentHp;

            updateStats(playerStats, consumable.statModifiers, true);
            if (oldHP > playerStats.currentHp) {
                ateBadFood = true;
            }

            toRemove.push_back(item);
            // printf("PLAYER STAT AFTER: \n");
            // printStats(playerStats);
            //LevelSystem->updateHealthbar(player);

        }
    }

    for (Entity& item : registry.equippableItems.entities) {
        if(!currentRoom->isEntityInRoom(item)){
            continue;
        }
        EquippableItem& equippable = registry.equippableItems.get(item);
        if (equippable.pickupCooldown > 0) {
            continue;
        }

        if (std::find(inventory.items.begin(), inventory.items.end(), item) == inventory.items.end()) {
            Motion& itemMotion = registry.motions.get(item);
            float distance = length(itemMotion.position - playerMotion.position);
            if (distance < PICKUP_RADIUS) {
                //printf("PLAYER STAT BEFORE: \n");
                //printStats(playerStats);

    /*            printf("ITEM STATS: \n");
                printStats(equippable.statModifiers)*/;
                currentMessage = "";
                if (inventory.items.size() >= 2) {
                    //printf("INVENTORY BEFORE: \n");
                    //for (int i = 0; i < inventory.items.size(); i++) {
                    //    printf("ITEM %d is: %d \n", i, inventory.items[i]);
                    //}
                    //printf("\n");
                    Entity old = inventory.items.front();
                    if(registry.uiElements.has(old)){
                        registry.uiElements.remove(old);
                    }
                    currentRoom->addRenderedEntity(old);
                    currentRoom->entity_render_requests[old] = registry.renderRequests.get(old);
                    inventory.items.erase(inventory.items.begin());

                    EquippableItem& oldItem = registry.equippableItems.get(old);
                    oldItem.pickupCooldown = 0.2f;
                    updateStats(playerStats, oldItem.statModifiers, false);
                    Motion& oldMotion = registry.motions.get(old);
                    oldMotion.position = itemMotion.position;
                    oldMotion.scale = itemMotion.scale;

                    //printf("PLAYER STAT AFTER ITEM REMOVAL: \n");
                    //printStats(playerStats);
                    droppedItem = true;

                }
                if(!registry.uiElements.has(item)){
                    registry.uiElements.emplace(item);
                }
                auto it = std::find(currentRoom->rendered_entities.begin(), currentRoom->rendered_entities.end(), item);
                if (it != currentRoom->rendered_entities.end()) {
                    currentRoom->rendered_entities.erase(it);
                }
                currentRoom->entity_render_requests.erase(item);
                updateStats(playerStats, equippable.statModifiers, true);
                inventory.items.push_back(item);

                //printf("PLAYER STAT AFTER NEW ITEM: \n");
                //printStats(playerStats);
                //printf("INVENTORY AFTER: \n");
                //for (int i = 0; i < inventory.items.size(); i++) {
                //    printf("ITEM %d is: %d \n", i, inventory.items[i]);
                //}
                //printf("\n");

                drawEquipped(player);

            }
        }

    }

    for (Entity& keyItem: registry.key.entities){
        if(!currentRoom->isEntityInRoom(keyItem)){
            continue;
        }
        if(!registry.motions.has(keyItem)){
            continue;
        }
        Motion& keyMotion = registry.motions.get(keyItem);
        float distance = length(keyMotion.position - playerMotion.position);
        if(distance < PICKUP_RADIUS) {
            keyInventory.keys.push_back(keyItem);
            toRemove.push_back(keyItem);
        }
    }

    for (Entity item : toRemove) {
        if (registry.renderRequests.has(item) && (registry.consumableItems.has(item) || registry.key.has(item) )) {
            registry.renderRequests.remove(item);
            if(registry.consumableItems.has(item)){
                registry.consumableItems.remove(item);
            }
            if(registry.key.has(item)){
                registry.key.remove(item);
            }
            auto it = std::find(currentRoom->rendered_entities.begin(), currentRoom->rendered_entities.end(), item);
            if (it != currentRoom->rendered_entities.end()) {
                currentRoom->rendered_entities.erase(it);
            }
        }
    }
}

void PlayState::updateStats(Stats &playerStats, Stats &itemStats, bool isAdding) {
    int adding = isAdding ? 1 : -1;
    if(currentMessage != "") {
        currentMessage += "\n";
    }

    if(itemStats.cuteness != 0) {
        currentMessage += "Cuteness: " + std::to_string(static_cast<int>(playerStats.cuteness)) + " -> " + std::to_string(static_cast<int>(playerStats.cuteness + itemStats.cuteness * adding)) + " \n";
    }
    playerStats.cuteness += itemStats.cuteness * adding;

    if(itemStats.ferocity != 0) {
        currentMessage += "Ferocity: " + std::to_string(static_cast<int>(playerStats.ferocity)) + " -> " + std::to_string(static_cast<int>(playerStats.ferocity + itemStats.ferocity * adding))  + " \n";
    }
    playerStats.ferocity += itemStats.ferocity * adding;

    if(itemStats.agility != 0) {
        currentMessage += "Agility: " + std::to_string(static_cast<int>(playerStats.agility)) + " -> " + std::to_string(static_cast<int>(playerStats.agility + itemStats.agility * adding))  + " \n";
    }
    playerStats.agility += itemStats.agility * adding;

    if(itemStats.maxHp != 0) {
        currentMessage += "Max Hp: " + std::to_string(static_cast<int>(playerStats.maxHp)) + " -> " + std::to_string(static_cast<int>(playerStats.maxHp + itemStats.maxHp * adding)) + " \n";
    }
    playerStats.maxHp += itemStats.maxHp * adding;

    if(itemStats.intelligence != 0) {
        currentMessage += "Intelligence: " + std::to_string(static_cast<int>(playerStats.intelligence)) + " -> " + std::to_string(static_cast<int>(playerStats.intelligence + itemStats.intelligence * adding)) + " \n";
    }
    playerStats.intelligence += itemStats.intelligence * adding;

    if(itemStats.reputation != 0) {
        currentMessage += "Reputation: " + std::to_string(static_cast<int>(playerStats.reputation)) + " -> " + std::to_string(static_cast<int>(playerStats.reputation + itemStats.reputation * adding)) + " \n";
    }
    playerStats.reputation += itemStats.reputation * adding;

    if(itemStats.stamina != 0) {
        currentMessage += "Stamina: " + std::to_string(static_cast<int>(playerStats.stamina)) + " -> " + std::to_string(static_cast<int>(playerStats.stamina + itemStats.stamina * adding)) + " \n";
    }
    playerStats.stamina += itemStats.stamina * adding;

    if(itemStats.stealth != 0) {
        currentMessage += "Stealth: " + std::to_string(static_cast<int>(playerStats.stealth)) + " -> " + std::to_string(static_cast<int>(playerStats.stealth + itemStats.stealth * adding)) + " \n";
    }
    playerStats.stealth += itemStats.stealth * adding;

    if(itemStats.currentHp != 0) {
        if(itemStats.currentHp < 0) {
            currentMessage += "Yuck! That food was bad! Gotta be careful with what you eat \nCurrent HP: " + std::to_string(playerStats.currentHp) + " -> " + std::to_string(playerStats.currentHp + itemStats.currentHp * adding) + " \n";
        } else {
            if(playerStats.currentHp + itemStats.currentHp > playerStats.maxHp){
                currentMessage += "Current HP: " + std::to_string(playerStats.currentHp) + " -> " + std::to_string(playerStats.maxHp) + " \n";
            } else {
                currentMessage += "Current HP: " + std::to_string(playerStats.currentHp) + " -> " + std::to_string(playerStats.currentHp + itemStats.currentHp * adding) + " \n";
            }
        }
    }

    if (playerStats.currentHp < playerStats.maxHp && itemStats.currentHp > 0) {
        if(playerStats.currentHp + itemStats.currentHp > playerStats.maxHp){
            playerStats.currentHp = playerStats.maxHp;
        } else {
            playerStats.currentHp += itemStats.currentHp * adding;
        }
    } else if (itemStats.currentHp < 0) {
        playerStats.currentHp += itemStats.currentHp * adding;
    }
}

void PlayState::printStats(const Stats& playerStats) {
    printf("STATS: \n");
    printf("CUTENESS : %f\n", playerStats.cuteness);
    printf("FEROCITY : %f\n", playerStats.ferocity);
    printf("HP : %d\n", playerStats.currentHp);
    printf("INTELLIGENCE : %f\n", playerStats.intelligence);
    printf("REPUTATION : %f\n", playerStats.reputation);
    printf("STAMINA : %d\n", playerStats.stamina);
    printf("STEALTH : %f\n", playerStats.stealth);
}

void PlayState::renderPickupText(WorldSystem *game) {
    for (Entity item: registry.equippableItems.entities) {
        EquippableItem& equippable = registry.equippableItems.get(item);

        if (equippable.isInteractable) {
            Room *currentRoom = game->get_level_manager()->currentLevel->currentRoom;
            Motion &playerMotion = registry.motions.get(player_character);

            if (std::find(currentRoom->rendered_entities.begin(), currentRoom->rendered_entities.end(), item) !=
                currentRoom->rendered_entities.end() && registry.renderRequests.has(item)) {
                Motion &itemMotion = registry.motions.get(item);
                float distance = length(itemMotion.position - playerMotion.position);
                // Only render text if the item is within a certain radius of the player
                if (distance < PICKUP_RADIUS) {
                    float text_x = itemMotion.position.x; // Adjust as needed
                    float text_y = (window_height_px - itemMotion.position.y + itemMotion.scale.y / 2);
                    // Adjust as needed
                    shouldPickUpItem = true;
                    shouldRenderEPress = true;
                }
            }
        }
    }
}

void PlayState::checkInNPCRange(WorldSystem *game) {
    shouldRenderEPress = false;
    shouldShowDialogue = false;
    for (Entity npc_entity: registry.npcs.entities) {
        Room *currentRoom = game->get_level_manager()->currentLevel->currentRoom;
        Motion &playerMotion = registry.motions.get(player_character);
        if (std::find(currentRoom->rendered_entities.begin(), currentRoom->rendered_entities.end(), npc_entity) != currentRoom
            ->rendered_entities.end()
            && (registry.npcs.get(npc_entity).hasDialogue || registry.npcs.get(npc_entity).isInteractable)) {
            Motion &npc_entityMotion = registry.motions.get(npc_entity);
            float distance = length(npc_entityMotion.position - playerMotion.position);
            // if(registry.doors.has(npc_entity)){
            //     if(registry.doors.get(npc_entity).required_keys == registry.keyInventory.get(player_character).keys.size()){
            //         shouldRenderEPress = false;
            //         shouldShowDialogue = false;
            //         continue;
            //     }
            // }
            if (distance < registry.npcs.get(npc_entity).interactDistance) {
                shouldRenderEPress = true;
                shouldShowDialogue = true;
                dialogueNPC = npc_entity;
                break;
            }
        }
    }
}

void PlayState::renderConsumeText(WorldSystem *game) {
    for (Entity item: registry.consumableItems.entities) {
        if (registry.consumableItems.get(item).isInteractable) {
            Room *currentRoom = game->get_level_manager()->currentLevel->currentRoom;
            Motion &playerMotion = registry.motions.get(player_character);
            if (std::find(currentRoom->rendered_entities.begin(), currentRoom->rendered_entities.end(), item) !=
                currentRoom->rendered_entities.end() && registry.renderRequests.has(item)) {
                Motion &itemMotion = registry.motions.get(item);
                float distance = length(itemMotion.position - playerMotion.position);

                // Only render text if the item is within a certain radius of the player
                if (distance < PICKUP_RADIUS) {
                    float text_x = itemMotion.position.x; // Adjust as needed
                    float text_y = (window_height_px - itemMotion.position.y + itemMotion.scale.y / 2);
                    // Adjust as needed
                    if (!registry.consumableItems.get(item).isBackpack) {
                        shouldPickUpItem = true;
                        shouldRenderEPress = true;
                    } else {
                        textRenderer->RenderText("Here is a backpack! Collide with it to grab it.", text_x, text_y,
                                                 defaultTextScale, {1.0f, 1.0f, 1.0f});
                        shouldPickUpItem = true;
                    }
                }
            }
        }
    }
}

void PlayState::renderKeyText(WorldSystem* game) {
    for (Entity item: registry.key.entities) {
        Room *currentRoom = game->get_level_manager()->currentLevel->currentRoom;
        Motion &playerMotion = registry.motions.get(player_character);
        if (std::find(currentRoom->rendered_entities.begin(), currentRoom->rendered_entities.end(), item) !=
            currentRoom->rendered_entities.end() && registry.renderRequests.has(item)) {
            Motion &itemMotion = registry.motions.get(item);
            float distance = length(itemMotion.position - playerMotion.position);

            // Only render text if the item is within a certain radius of the player
            if (distance < PICKUP_RADIUS) {
                float text_x = itemMotion.position.x; // Adjust as needed
                float text_y = (window_height_px - itemMotion.position.y + itemMotion.scale.y / 2);
                // Adjust as needed
                shouldPickUpItem = true;
                shouldRenderEPress = true;
            }
        }
    }
}

void PlayState::renderDialogueText(WorldSystem *game) {
    if(currentMessage != "") {
        if(!registry.renderRequests.has(dialogueBox)) {
            Motion& motion = registry.motions.emplace(dialogueBox);
            motion.scale = {window_width_px*0.9f, window_height_px/5.0f};
            motion.position = {window_width_px/2.0f, window_height_px - window_height_px/9.0f};
            motion.velocity = {0, 0};

            RenderRequest dialogueBox_request = {
                TEXTURE_ASSET_ID::DIALOGUE_BOX,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                {},
                false
            };

            registry.renderRequests.insert(dialogueBox, dialogueBox_request);

            registry.uiElements.emplace(dialogueBox);

            Motion& e_press_motion = registry.motions.emplace(dialogueEPress);
            e_press_motion.scale = {motion.scale.x/25.0f, motion.scale.y/5.0f};
            e_press_motion.position = {motion.position.x + motion.scale.x/2.0f - e_press_motion.scale.x,
            motion.position.y + motion.scale.y/2.0f - e_press_motion.scale.y};
            e_press_motion.velocity = {0, 0};
            e_press_motion.z = 1;

            dialogue_e_animation.frameCount = 3;
            dialogue_e_animation.currentFrame = 0;
            dialogue_e_animation.frameTime = 300.0f;
            dialogue_e_animation.elapsedTime = 0.0f;
            dialogue_e_animation.columns = 3;
            dialogue_e_animation.rows = 1;
            dialogue_e_animation.startRow= 0;
            dialogue_e_animation.startCol = 0;

            dialogue_e_request = {
                TEXTURE_ASSET_ID::E_PRESS,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                dialogue_e_animation,
                true
            };

            registry.renderRequests.insert(dialogueEPress, dialogue_e_request);
        }


        float base_text_scale = 0.0004;
        float text_scale = base_text_scale * window_height_px;
        Motion& dialogue_box_m = registry.motions.get(dialogueBox);

        GLfloat box_width = dialogue_box_m.scale.x * 0.8f;  // 80% of the box's width
        GLfloat box_height = dialogue_box_m.scale.y * 0.8f; // 80% of the box's height

        // Adjust to render from the top left corner of the box
        GLfloat text_x = dialogue_box_m.position.x - box_width / 1.8f;
        GLfloat text_y = window_height_px - dialogue_box_m.position.y + box_height * 0.3; // Start near the top edge

        // Render the text inside the box
        textRenderer->RenderBoxedText(currentMessage, text_x, text_y, text_scale, glm::vec3(1.0f, 1.0f, 1.0f), box_width, box_height);
    } else {
        if(registry.renderRequests.has(dialogueBox)) {
            registry.remove_all_components_of(dialogueBox);
        }

        if(registry.renderRequests.has(dialogueEPress)) {
            registry.remove_all_components_of(dialogueEPress);
        }
    }
}

void PlayState::renderSelectBox(WorldSystem *game) {
    if(shouldShowSelect) {
        if(!registry.renderRequests.has(selectBox)) {
            Motion& motion = registry.motions.emplace(selectBox);
            motion.scale = {window_width_px * 0.19f, window_height_px/7.0f};
            motion.position = {window_width_px - window_width_px/6.5f, window_height_px - window_height_px/9.0f - window_height_px/5.5f};
            motion.velocity = {0, 0};

            RenderRequest selectBox_request = {
                TEXTURE_ASSET_ID::DIALOGUE_BOX,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                {},
                false
            };

            registry.renderRequests.insert(selectBox, selectBox_request);

            registry.uiElements.emplace(selectBox);
        }

        float base_text_scale = 0.00055;
        float text_scale = base_text_scale * window_height_px;
        Motion& select_m = registry.motions.get(selectBox);

        GLfloat box_width = select_m.scale.x * 0.8f;  // 80% of the box's width
        GLfloat box_height = select_m.scale.y * 0.7f; // 80% of the box's height

        GLfloat text_x = select_m.position.x - box_width / 1.8f;
        GLfloat text_y = window_height_px - select_m.position.y + box_height * 0.2;

        textRenderer->RenderBoxedText("Yes", text_x, text_y, text_scale, glm::vec3(1.0f, 1.0f, 1.0f), box_width, box_height);

        text_y = window_height_px - select_m.position.y - box_height * 0.4;

        textRenderer->RenderBoxedText("No", text_x, text_y, text_scale, glm::vec3(1.0f, 1.0f, 1.0f), box_width, box_height);
    } else {
        if(registry.renderRequests.has(selectBox)) {
            registry.remove_all_components_of(selectBox);
        }
    }
}

void PlayState::renderSelectArrow(WorldSystem* game) {
    if(shouldShowSelect) {
        if(!registry.renderRequests.has(selectArrow)) {
            Motion& motion = registry.motions.emplace(selectArrow);
            motion.scale = {window_width_px * 0.02, window_width_px * 0.02};
            motion.position = {window_width_px - window_width_px/8.5f, 0};
            motion.velocity = {0, 0};
            motion.z = 1;

            RenderRequest selectArrow_request = {
                TEXTURE_ASSET_ID::SELECT_ARROW,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                {},
                false
            };

            registry.renderRequests.insert(selectArrow, selectArrow_request);
            registry.uiElements.emplace(selectArrow);
        }
    } else {
        if(registry.renderRequests.has(selectArrow)) {
            registry.remove_all_components_of(selectArrow);
            selectArrowPos = 1;
        }
    }
}


void PlayState::buryPlayer(Entity player, Room* currentRoom) {
    Motion& motion = registry.motions.get(player);
    vec2 death_position = motion.position;
    motion.position -= vec2{50, 0}; // Move the player down to the ground
    motion.velocity = {0, 0}; // Stop the player from moving

    Stats& player_stats = registry.stats.get(player);

    // Create a tombstone at the player's death position
    Entity tombstone = registry.create_entity();
    Motion& tombstone_motion = registry.motions.emplace(tombstone);
    tombstone_motion.position = death_position;
    tombstone_motion.scale = {ITEM_WIDTH * 2, ITEM_HEIGHT * 2};

    RenderRequest render_request = {
        TEXTURE_ASSET_ID::TOMBSTONE,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };
    currentRoom->addRenderedEntity(tombstone);
    currentRoom->entity_render_requests[tombstone] = render_request;
    registry.renderRequests.emplace(tombstone, render_request);

	if (!registry.inventory.has(player)) {
		return; // No inventory to process
	}

    // Drop all items below the tombstone
    if (registry.inventory.has(player)) {
        Inventory& inventory = registry.inventory.get(player);
        vec2 drop_position = death_position - vec2{25, -50}; // Starting position below the tombstone

        for (Entity item : inventory.items) {
            if (!registry.motions.has(item)) {
                registry.motions.emplace(item);
            }
            Motion& item_motion = registry.motions.get(item);
            item_motion.position = drop_position;
            item_motion.scale = vec2({ ITEM_WIDTH, ITEM_HEIGHT });
        	EquippableItem& oldItem = registry.equippableItems.get(item);
        	TEXTURE_ASSET_ID textureId = registry.renderRequests.has(item)
											 ? registry.renderRequests.get(item).used_texture
											 : TEXTURE_ASSET_ID::TEXTURE_COUNT;
        	registry.motions.get(item).z = 1.0f;
        	drop_position += vec2{10, 0};
        	RenderRequest item_render_request = {
        		textureId,
				EFFECT_ASSET_ID::TEXTURED,
				GEOMETRY_BUFFER_ID::SPRITE,
				{},
				false
			};
            registry.uiElements.remove(item);
            updateStats(player_stats, oldItem.statModifiers, false);
            // Add the item back to the current room
            currentRoom->addRenderedEntity(item); // makes the item pickable
        	currentRoom->entity_render_requests[item] = item_render_request;
            drop_position = drop_position + vec2{25, 0};
        }
        inventory.items.clear();
    }
}

void PlayState::createKey(vec2 position) {
    Entity entity = registry.create_entity();
    Motion& motion = registry.motions.emplace(entity);
    motion.position = position;
    motion.z = 1;
    motion.scale = {ITEM_WIDTH, ITEM_HEIGHT};

    Key& key = registry.key.emplace(entity);
    key.fadeTimer = 2;

    RenderRequest keyRender = {
        TEXTURE_ASSET_ID::KEY,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false,
        0
    };

    level_manager->currentLevel->currentRoom->addRenderedEntity(entity);
    level_manager->currentLevel->currentRoom->entity_render_requests[entity] = keyRender;
}

void PlayState::drawKey(){
    KeyInventory& keyInventory = registry.keyInventory.get(player_character);
    vec2 keyIconPosition = {window_width_px - window_width_px / 10, window_height_px / 3.5};
    vec2 keyTextPosition = {keyIconPosition.x + 30, window_height_px - keyIconPosition.y};

    RenderRequest keyIconRender = {
        TEXTURE_ASSET_ID::KEY,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };


    if (!registry.uiElements.has(keyIconEntity)) {
        keyIconEntity = registry.create_entity();
        registry.uiElements.emplace(keyIconEntity);
    }
    if(!registry.renderRequests.has(keyIconEntity)){
        registry.renderRequests.insert(keyIconEntity, keyIconRender);
    }
    if(!registry.motions.has(keyIconEntity)){
        Motion keyIconMotion = {};
        keyIconMotion.position = keyIconPosition;
        keyIconMotion.scale = {50, 50};
        keyIconMotion.z = 1.0f;
        registry.motions.insert(keyIconEntity, keyIconMotion);
    }


    std::string keyCountText = "x" + std::to_string(keyInventory.keys.size());
    textRenderer->RenderText(keyCountText, keyTextPosition.x, keyTextPosition.y, 0.5, {1.0f, 1.0f, 1.0f});
}