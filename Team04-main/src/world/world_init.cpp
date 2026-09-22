#include "world/world_init.hpp"
#include "core/ecs_registry.hpp"
#include "core/ecs.hpp"
#include <iostream>
#include <vector>
#include "world_init.hpp"


const float PATROL_SPEED = 100;


Entity createPlayer(vec2 position, float tile_width, float tile_height) {
    Entity entity = registry.create_entity();

    Motion& motion = registry.motions.emplace(entity);
    motion.velocity = { 0, 0 };
    motion.position = position;

    // Set player scale based on room tile dimensions
    motion.scale = { tile_width, tile_height };

    motion.angle = 0.f;
    registry.players.emplace(entity);

    BoundingBox& bounds = registry.boundingBoxes.emplace(entity);
    bounds.width = tile_width / 2;  // Scale bounding box based on tile size
    bounds.height = tile_height / 2;

    Collider& collider = registry.colliders.emplace(entity);
    collider.type = PLAYER;

    Inventory& inventory = registry.inventory.emplace(entity);
    KeyInventory& keyInventory = registry.keyInventory.emplace(entity);
    Stats& stats = registry.stats.emplace(entity);
    stats.maxHp = 20;
    stats.currentHp = 17;
    stats.cuteness = 1;
    stats.agility = 3;
    stats.ferocity = 4;
    stats.reputation = 0;

    // std::cout << "Player id:" << entity << std::endl;

    return entity;

}

std::pair<Entity, Entity> createBackpack(RenderSystem* renderer, vec2 pos) {
    Entity meshEntity = registry.create_entity();

    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::BACKPACK);
    registry.meshPtrs.emplace(meshEntity, &mesh);

    Motion& motion = registry.motions.emplace(meshEntity);
    motion.velocity = { 0, 0 };
    motion.position = pos;
    motion.scale = vec2({ 50, 50 });
    motion.z = 0.0f; // mesh should be slightly behind the texture

    BoundingBox& bb = registry.boundingBoxes.emplace(meshEntity);
    bb.height = motion.scale.x * 4; // for robustness to make sure mesh collision wroks properly
    bb.width = motion.scale.y * 4;

    Collider& collider = registry.colliders.emplace(meshEntity);
    collider.type = ITEM;

    registry.consumableItems.emplace(meshEntity);
    registry.consumableItems.get(meshEntity).isBackpack = true;

    Entity textureEntity = registry.create_entity();
    Motion& dummyMotion = registry.motions.emplace(textureEntity);
    dummyMotion = motion;
    motion.scale = vec2({ 50, -50 });
    registry.uiElements.emplace(textureEntity); // easy fix to render the texture over the mesh.
    dummyMotion.z = 1.0f; // texture should be in front of the mesh

    return { meshEntity, textureEntity };
}


Entity createPatrol(vec2 position, float ms_to_turn, float tile_width, float tile_height) {
    Entity patrol_entity = registry.create_entity();

    Motion& motion = registry.motions.emplace(patrol_entity);
    motion.position = position;
    motion.velocity = { 0, 0 };
    motion.scale = { tile_width, tile_height };
    motion.speed = PATROL_SPEED;
    motion.angle = M_PI / 2;

    Patrol& patrol = registry.patrols.emplace(patrol_entity);

    patrol.light = registry.create_entity();

    Motion& lightMotion = registry.motions.emplace(patrol.light);
    lightMotion.position = position;
    lightMotion.position.y += CONE_HEIGHT / 2.f + 39.f; // hardcoded magic number
    lightMotion.velocity = { 0, 0 };
    lightMotion.scale = vec2({ CONE_WIDTH, CONE_HEIGHT });
    lightMotion.speed = 0;

    registry.rotatables.emplace(patrol.light);

    RandomWalker& rnd_walker = registry.randomWalkers.emplace(patrol_entity);
    rnd_walker.sec_to_turn = ms_to_turn;

    Collider& collider = registry.colliders.emplace(patrol_entity);
    collider.type = PATROL;

    BoundingBox& bb = registry.boundingBoxes.emplace(patrol_entity);
    bb.height = tile_height;
    bb.width = tile_width;

    return patrol_entity;
}

void addPatrolToRoom(RenderSystem *renderer, Room *lobby, Entity patrol) {
    lobby->addRenderedEntity(patrol);
    lobby->addNonRenderedEntity(patrol);

    RenderRequest patrol_render_request;
    patrol_render_request.used_texture = TEXTURE_ASSET_ID::PATROL;
    patrol_render_request.used_effect = EFFECT_ASSET_ID::TEXTURED;
    patrol_render_request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    patrol_render_request.hasAnimation = true;
    renderer->setAnimation(patrol_render_request, AnimationState::IDLE, renderer->npcAnimationMap);

    int patrol_id = patrol;
    lobby->entity_render_requests[patrol_id] = patrol_render_request;

    Patrol& patrolData = registry.patrols.get(patrol);
    Entity lightCone = patrolData.light;

    lobby->addRenderedEntity(lightCone);

    RenderRequest lightCone_render_request = {
        TEXTURE_ASSET_ID::LIGHT_CONE,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };
    lobby->entity_render_requests[lightCone] = lightCone_render_request;
}

Entity createNPC(vec2 position, float tile_width, float tile_height, Stats stats, std::string npc_name, bool isInteractable, float attackDuration, TEXTURE_ASSET_ID encounter_texture) {
    Entity npc_entity = registry.create_entity();

    Motion& motion = registry.motions.emplace(npc_entity);
    motion.position = position;
    motion.velocity = { 0, 0 };
    motion.scale = { tile_width, tile_height };
    motion.speed = PATROL_SPEED;
    motion.angle = M_PI / 2;

    NPC& npc = registry.npcs.emplace(npc_entity);
    npc.name = npc_name;
    npc.encounter_texture_id = (int)encounter_texture;
    npc.isInteractable = isInteractable;
    npc.attackDuration = attackDuration;
    Stats& npc_stats = registry.stats.emplace(npc_entity);
    npc_stats = stats;

    BoundingBox& bb = registry.boundingBoxes.emplace(npc_entity);
    bb.height = tile_height;
    bb.width = tile_width;

    Collider& collider = registry.colliders.emplace(npc_entity);
    collider.type = CREATURE;

    return npc_entity;
}

void addNPCToRoom(RenderSystem *renderer, Room *lobby, Entity npc, TEXTURE_ASSET_ID texture, AnimationState state, std::unordered_map<AnimationState, Animation> animationMap) {
    lobby->addRenderedEntity(npc);
    lobby->addNonRenderedEntity(npc);

    RenderRequest npc_render_request;
    npc_render_request.used_texture = texture;
    npc_render_request.used_effect = EFFECT_ASSET_ID::TEXTURED;
    npc_render_request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    npc_render_request.hasAnimation = true;
    renderer->setAnimation(npc_render_request, state, animationMap);

    int npc_id = npc;
    lobby->entity_render_requests[npc_id] = npc_render_request;
}

Entity createConsumable(vec2 position, Stats stats, TEXTURE_ASSET_ID texture) {
    Entity item = registry.create_entity();
    Motion& itemMotion = registry.motions.emplace(item);
    itemMotion.position = position;
    itemMotion.scale = vec2({ ITEM_WIDTH, ITEM_HEIGHT });
    ConsumableItem& consumable = registry.consumableItems.emplace(item);
    consumable.isInteractable = true;
    consumable.statModifiers = stats;
    return item;
}

Entity createEquippable(vec2 position, Stats stats, TEXTURE_ASSET_ID texture) {
    Entity item = registry.create_entity();
    Motion& itemMotion = registry.motions.emplace(item);
    itemMotion.position = position;
    itemMotion.scale = vec2({ ITEM_WIDTH, ITEM_HEIGHT });
    EquippableItem& equippable = registry.equippableItems.emplace(item);
    equippable.isInteractable = true;
    equippable.statModifiers = stats;
    equippable.texture = texture;
    return item;

}

float tile_width;
float tile_height;
int grid_columns; // Total columns in the grid
int grid_rows;     // Total rows in the grid
float scale_x;
float scale_y;
float room_scale;
float scaled_tile_width;
float scaled_tile_height;
float offset_x;
float offset_y;
int window_width, window_height;
// Entrances //
int base_to_tunnel_id;
int tunnel_to_base_id;
int tunnel_to_map_id;
int map_to_tunnel_id;
int lobby_to_map_id;
int map_to_lobby_id;
int office_to_map_id;
int map_to_office_id;
int vet_to_map_id;
int map_to_vet_id;
int lobby_to_boss_id;
int boss_to_lobby_id;
int player_id;

void createKennelRoom(Level* level, RenderSystem* renderer) {
    Room* base = new Room("kennel room");
   // ****************************************************//
    // Kennel Room //
    base->gridDimensions = { 16, 9 };

    // Define tile dimensions based on the new 16x9 grid (48x48 virtual tiles)
    float tile_width = 48.0f;
    float tile_height = 48.0f;
    int grid_columns = 16; // Total columns in the grid
    int grid_rows = 9;     // Total rows in the grid

    // Calculate scaling factors
    scale_x = window_width / (tile_width * grid_columns);
    scale_y = window_height / (tile_height * grid_rows);
    room_scale = std::min(scale_x, scale_y);

    // Apply scaling factor to tile dimensions for consistent rendering
    scaled_tile_width = tile_width * room_scale;
    scaled_tile_height = tile_height * room_scale;

    // Center the room in the window
    offset_x = (window_width - (scaled_tile_width * grid_columns)) / 2.0f;
    offset_y = (window_height - (scaled_tile_height * grid_rows)) / 2.0f;

    base->player_position = { scaled_tile_width / 6 + 3 * scaled_tile_width, scaled_tile_height * 2 };
    level->originalSpawnPositions.push_back(base->player_position);
    base->player_scale = { window_width_px / base->gridDimensions.x, window_height_px / base->gridDimensions.y };

    for (int row = 0; row < grid_rows; row++) {
        Entity left_side_wall = registry.create_entity();
        Motion& left_side_motion = registry.motions.emplace(left_side_wall);

        left_side_motion.position = { scaled_tile_width / 12, row * scaled_tile_height };
        left_side_motion.scale = { scaled_tile_width / 6, scaled_tile_height * 2 };
        left_side_motion.velocity = { 0, 0 };

        base->addRenderedEntity(left_side_wall);

        RenderRequest left_side_wall_request = {
            TEXTURE_ASSET_ID::SIDE_WALL_MID,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
            {},
            false
        };

        int left_side_id = left_side_wall;
        base->entity_render_requests[left_side_id] = left_side_wall_request;

        if (row == 0) {
            for (int col = 0; col < grid_columns; col += 2) {
                Entity back_wall = registry.create_entity();
                Motion& back_wall_motion = registry.motions.emplace(back_wall);

                back_wall_motion.position = { scaled_tile_width / 6 + col * scaled_tile_width + scaled_tile_width, scaled_tile_height / 2 };
                back_wall_motion.scale = { scaled_tile_width * 2, scaled_tile_height };
                back_wall_motion.velocity = { 0, 0 };

                base->addRenderedEntity(back_wall);

                RenderRequest back_wall_request = {
                    TEXTURE_ASSET_ID::TOP_WALL,
                    EFFECT_ASSET_ID::TEXTURED,
                    GEOMETRY_BUFFER_ID::SPRITE,
                    {},
                    false
                };

                int back_wall_id = back_wall;
                base->entity_render_requests[back_wall_id] = back_wall_request;

                if (col == 2) {
                    Entity wall_hole = registry.create_entity();
                    Motion& wall_hole_motion = registry.motions.emplace(wall_hole);
                    Door& wall_hole_door = registry.doors.emplace(wall_hole);

                    wall_hole_motion.position = { scaled_tile_width / 6 + col * scaled_tile_width + scaled_tile_width * 1.5, scaled_tile_height / 2 + scaled_tile_height / 6 };
                    wall_hole_motion.scale = { scaled_tile_width * 0.8, scaled_tile_height * 0.8 };
                    wall_hole_motion.velocity = { 0, 0 };

                    Collider& wall_hole_collider = registry.colliders.emplace(wall_hole);
                    wall_hole_collider.type = DOOR;

                    BoundingBox& wall_hole_bb = registry.boundingBoxes.emplace(wall_hole);
                    wall_hole_bb.width = scaled_tile_width * 0.5f;
                    wall_hole_bb.height = scaled_tile_height * 0.8f;

                    base->addRenderedEntity(wall_hole);
                    base->addNonRenderedEntity(wall_hole);
                    RenderRequest wall_hole_request = {
                        TEXTURE_ASSET_ID::WALL_HOLE,
                        EFFECT_ASSET_ID::TEXTURED,
                        GEOMETRY_BUFFER_ID::SPRITE,
                        {},
                        false
                    };

                    base_to_tunnel_id = wall_hole;
                    base->entity_render_requests[base_to_tunnel_id] = wall_hole_request;

                    level->spawnPositions[base_to_tunnel_id] = { scaled_tile_width / 6 + col * scaled_tile_width + scaled_tile_width * 1.5, scaled_tile_height / 2 + scaled_tile_height / 6 + scaled_tile_height / 1.5 };
                }
            }
        }
        else {
            for (int col = 0; col < grid_columns; col++) {
                Entity tile = registry.create_entity();
                Motion& tile_motion = registry.motions.emplace(tile);

                tile_motion.position = { scaled_tile_width / 6 + (col * scaled_tile_width) + scaled_tile_width / 2, (row * scaled_tile_height) + scaled_tile_height / 2 };
                tile_motion.scale = { scaled_tile_width, scaled_tile_height };
                tile_motion.velocity = { 0,0 };

                base->addRenderedEntity(tile);
                TEXTURE_ASSET_ID texture_id;
                if (row == 1) {
                    texture_id = TEXTURE_ASSET_ID::TOP_FLOOR;
                }
                else {
                    texture_id = TEXTURE_ASSET_ID::MID_FLOOR;
                }

                RenderRequest tile_request = {
                    texture_id,
                    EFFECT_ASSET_ID::TEXTURED,
                    GEOMETRY_BUFFER_ID::SPRITE,
                    {},
                    false
                };

                int tile_id = tile;
                base->entity_render_requests[tile_id] = tile_request;
            }
        }

        Entity right_side_wall = registry.create_entity();
        Motion& right_side_motion = registry.motions.emplace(right_side_wall);

        right_side_motion.position = { window_width_px - scaled_tile_width / 12, row * scaled_tile_height };
        right_side_motion.scale = { scaled_tile_width / 6, scaled_tile_height * 2 };
        right_side_motion.velocity = { 0, 0 };

        base->addRenderedEntity(right_side_wall);

        RenderRequest right_side_wall_request = {
            TEXTURE_ASSET_ID::SIDE_WALL_MID,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
            {},
            false
        };

        int right_side_id = right_side_wall;
        base->entity_render_requests[right_side_id] = right_side_wall_request;
    }

    Entity player2 = registry.players.entities[0];
    player_id = player2.getId();
    base->addRenderedEntity(player2);
    base->addNonRenderedEntity(player2);

    RenderRequest player_request_1;
    player_request_1.used_texture = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    player_request_1.used_effect = EFFECT_ASSET_ID::TEXTURED;
    player_request_1.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    player_request_1.hasAnimation = true;
    renderer->setAnimation(player_request_1, AnimationState::IDLE, renderer->catAnimationMap);
    base->entity_render_requests[player_id] = player_request_1;

    //Top Jail Cells
    for (int i = 0; i < 6; i++) {
        Entity jail_wall = registry.create_entity();
        Motion& jail_wall_motion = registry.motions.emplace(jail_wall);

        jail_wall_motion.position = { scaled_tile_width / 6 + scaled_tile_width * 2 * (i + 1) - scaled_tile_width / 12, scaled_tile_height / 2 + scaled_tile_height };
        jail_wall_motion.scale = { scaled_tile_width / 6, scaled_tile_height * 2.5 };
        jail_wall_motion.velocity = { 0, 0 };

        base->addRenderedEntity(jail_wall);

        RenderRequest jail_wall_request = {
            TEXTURE_ASSET_ID::INNER_WALL,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
            {},
            false
        };

        int jail_wall_id = jail_wall;
        base->entity_render_requests[jail_wall_id] = jail_wall_request;
    }

    for (int i = 0; i < 12; i++) {
        Entity cell = registry.create_entity();
        Motion& cell_motion = registry.motions.emplace(cell);

        cell_motion.position = { scaled_tile_width / 6 + i * scaled_tile_width + scaled_tile_width / 2, scaled_tile_height * 3 };
        cell_motion.scale = { scaled_tile_width, scaled_tile_height };
        cell_motion.velocity = { 0, 0 };

        base->addRenderedEntity(cell);

        TEXTURE_ASSET_ID texture_id;
        if (i % 2 == 0) {
            texture_id = TEXTURE_ASSET_ID::JAIL_WALL;
        }
        else {
            texture_id = TEXTURE_ASSET_ID::JAIL_DOOR;
        }

        RenderRequest cell_request = {
            texture_id,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
            {},
            false
        };

        int cell_id = cell;
        base->entity_render_requests[cell_id] = cell_request;
    }

    //Bottom Jail Cells
    for (int i = 0; i < 12; i++) {
        Entity cell = registry.create_entity();
        Motion& cell_motion = registry.motions.emplace(cell);

        cell_motion.position = { scaled_tile_width / 6 + i * scaled_tile_width + scaled_tile_width / 2, scaled_tile_height * 6 };
        cell_motion.scale = { scaled_tile_width, scaled_tile_height };
        cell_motion.velocity = { 0, 0 };

        base->addRenderedEntity(cell);

        TEXTURE_ASSET_ID texture_id;
        if (i % 2 == 0) {
            texture_id = TEXTURE_ASSET_ID::JAIL_DOOR;
        }
        else {
            texture_id = TEXTURE_ASSET_ID::JAIL_WALL;
        }

        RenderRequest cell_request = {
            texture_id,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
            {},
            false
        };

        int cell_id = cell;
        base->entity_render_requests[cell_id] = cell_request;
    }

    for (int i = 0; i < 6; i++) {
        Entity jail_wall = registry.create_entity();
        Motion& jail_wall_motion = registry.motions.emplace(jail_wall);

        jail_wall_motion.position = { scaled_tile_width / 6 + scaled_tile_width * 2 * (i + 1) - scaled_tile_width / 12, scaled_tile_height / 4 + scaled_tile_height * 7 };
        jail_wall_motion.scale = { scaled_tile_width / 6, scaled_tile_height * 3.5 };
        jail_wall_motion.velocity = { 0, 0 };

        base->addRenderedEntity(jail_wall);

        RenderRequest jail_wall_request = {
            TEXTURE_ASSET_ID::INNER_WALL,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
            {},
            false
        };

        int jail_wall_id = jail_wall;
        base->entity_render_requests[jail_wall_id] = jail_wall_request;
    }

    // for(int col = 0; col < grid_columns; col++) {
    //     Entity bottom_wall = registry.create_entity();
    //     Motion& bottom_motion = registry.motions.emplace(bottom_wall);

    //     bottom_motion.position = {col * scaled_tile_width + scaled_tile_width/2, window_height_px - scaled_tile_height/12};
    //     bottom_motion.scale = {scaled_tile_width*2, scaled_tile_height/6};
    //     bottom_motion.velocity = {0, 0};

    //     base->addRenderedEntity(bottom_wall);

    //     RenderRequest bottom_wall_request = {
    //         TEXTURE_ASSET_ID::BOT_WALL,
    //         EFFECT_ASSET_ID::TEXTURED,
    //         GEOMETRY_BUFFER_ID::SPRITE,
    //         {},
    //         false
    //     };

    //     int bottom_id = bottom_wall;
    //     base->entity_render_requests[bottom_id] = bottom_wall_request;
    // }

    Entity pillow_1 = registry.create_entity();
    Motion& pillow_1_motion = registry.motions.emplace(pillow_1);

    pillow_1_motion.position = { scaled_tile_width / 6 + scaled_tile_width / 1.9, scaled_tile_height * 1.4 };
    pillow_1_motion.scale = { scaled_tile_width * 0.7, scaled_tile_height * 0.7 };
    pillow_1_motion.velocity = { 0, 0 };

    base->addRenderedEntity(pillow_1);
    RenderRequest pillow_1_request = {
        TEXTURE_ASSET_ID::PILLOW_BLUE,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    int pillow_1_id = pillow_1;
    base->entity_render_requests[pillow_1_id] = pillow_1_request;

    Entity old_cat = registry.create_entity();
    registry.npcs.emplace(old_cat);
    registry.npcs.get(old_cat).hasDialogue = true;
    registry.npcs.get(old_cat).name = "Cat Lying";

    registry.npcs.get(old_cat).dialogue.push("Old Cat: You've finally woken up huh");
    registry.npcs.get(old_cat).dialogue.push("Old Cat: You don't remember where you are?");
    registry.npcs.get(old_cat).dialogue.push("Old Cat: You got captured by the patrols and was sent here");
    registry.npcs.get(old_cat).dialogue.push("Old Cat: They're trying to euthanize all of the stray animals around");
    registry.npcs.get(old_cat).dialogue.push("Old Cat: There's a tunnel in your room for you to escape from if you wish");
    registry.npcs.get(old_cat).dialogue.push("Old Cat: But be careful not to run into any patrols");
    registry.npcs.get(old_cat).dialogue.push("Old Cat: If they spot you in their flashlight, they'll chase you down and you'll lose a life");
    registry.npcs.get(old_cat).dialogue.push("Old Cat: And who knows how many lives you have until it's over for you");

    registry.npcs.get(old_cat).interactDistance = 250.0f;
    Motion& old_cat_motion = registry.motions.emplace(old_cat);

    old_cat_motion.position = { scaled_tile_width , scaled_tile_height * 1.3 };
    old_cat_motion.scale = { scaled_tile_width * 0.9, scaled_tile_height * 0.9 };
    old_cat_motion.velocity = { 0, 0 };
    Animation old_cat_animation;
    old_cat_animation.frameCount = 1;
    old_cat_animation.currentFrame = 0;
    old_cat_animation.frameTime = 0.0f;
    old_cat_animation.elapsedTime = 0.0f;
    old_cat_animation.columns = 32;
    old_cat_animation.rows = 17;
    old_cat_animation.startRow= 15;
    old_cat_animation.startCol = 10;

    base->addRenderedEntity(old_cat);
    RenderRequest old_cat_request = {
        TEXTURE_ASSET_ID::BEIGE_CAT,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        old_cat_animation,
        true
    };

    int old_cat_id = old_cat;
    base->entity_render_requests[old_cat_id] = old_cat_request;

    Entity old_cat_dialogue_icon = registry.create_entity();
    Motion& dialogue_icon_motion = registry.motions.emplace(old_cat_dialogue_icon);

    dialogue_icon_motion.position = {old_cat_motion.position.x, old_cat_motion.position.y - old_cat_motion.scale.y/2.0f};
    dialogue_icon_motion.scale = {scaled_tile_width * 0.4, scaled_tile_height * 0.4};
    dialogue_icon_motion.velocity = {0, 0};

    registry.uiElements.emplace(old_cat_dialogue_icon);

    base->addRenderedEntity(old_cat_dialogue_icon);
    RenderRequest old_cat_dialogue_request  = {
        TEXTURE_ASSET_ID::DIALOGUE_ICON,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    int old_cat_dialogue_icon_id = old_cat_dialogue_icon;
    base->entity_render_requests[old_cat_dialogue_icon_id] = old_cat_dialogue_request;

    Entity cat2 = registry.create_entity();
    registry.npcs.emplace(cat2);
    Motion& cat2_motion = registry.motions.emplace(cat2);

    cat2_motion.position = { scaled_tile_width * 9, scaled_tile_height * 7 };
    cat2_motion.scale = { -scaled_tile_width, scaled_tile_height };
    cat2_motion.velocity = { 0, 0 };

    Animation cat2_animation;
    cat2_animation.frameCount = 4;
    cat2_animation.currentFrame = 0;
    cat2_animation.frameTime = 1000.0f;
    cat2_animation.elapsedTime = 0.0f;
    cat2_animation.columns = 24;
    cat2_animation.rows = 17;
    cat2_animation.startRow= 4;
    cat2_animation.startCol = 8;
    RenderRequest cat2_render = {
        TEXTURE_ASSET_ID::ORANGE_CAT,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        cat2_animation,
        true
    };

    base->addRenderedEntity(cat2);
    int cat2_id = cat2.getId();
    base->entity_render_requests[cat2_id] = cat2_render;

    Stats stats;
    Entity dog = createNPC({scaled_tile_width * 7, scaled_tile_height * 1.3}, tile_width * 3, tile_height * 3, stats, "dog_notmoving", false, 0.0f, TEXTURE_ASSET_ID::TEXTURE_COUNT);
    addNPCToRoom(renderer, base, dog, TEXTURE_ASSET_ID::BROWN_DOG, AnimationState::LAYING_DOWN, renderer->dogAnimationMap);

    Entity dog2 = createNPC({scaled_tile_width * 5, scaled_tile_height * 7}, tile_width * 3, tile_height * 3, stats, "dog_notmoving", false, 0.0f, TEXTURE_ASSET_ID::TEXTURE_COUNT);
    addNPCToRoom(renderer, base, dog2, TEXTURE_ASSET_ID::WHITE_DOG, AnimationState::SLEEPING,  renderer->dogAnimationMap);

    // Bounding boxes //
    Entity top_jail_bounds = registry.create_entity();
    Motion& top_jail_motion = registry.motions.emplace(top_jail_bounds);

    top_jail_motion.position = { scaled_tile_width / 6 + 6 * scaled_tile_width, scaled_tile_height * 3.33 };
    top_jail_motion.scale = { scaled_tile_width * 12, scaled_tile_height / 2.2 };
    top_jail_motion.velocity = { 0, 0 };

    Collider& top_jail_collider = registry.colliders.emplace(top_jail_bounds);
    top_jail_collider.type = OBSTACLE;
    top_jail_collider.transparent = true;

    BoundingBox& top_jail_bb = registry.boundingBoxes.emplace(top_jail_bounds);
    top_jail_bb.width = scaled_tile_width * 12;
    top_jail_bb.height = scaled_tile_height / 1.7f;

    base->addNonRenderedEntity(top_jail_bounds);

    Entity bottom_jail_bounds = registry.create_entity();
    Motion& bottom_jail_motion = registry.motions.emplace(bottom_jail_bounds);

    bottom_jail_motion.position = {scaled_tile_width/6 + 6 * scaled_tile_width, scaled_tile_height * 6.33};
    bottom_jail_motion.scale = {scaled_tile_width * 12, scaled_tile_height/2.2};
    bottom_jail_motion.velocity = {0, 0};

    Collider& bottom_jail_collider = registry.colliders.emplace(bottom_jail_bounds);
    bottom_jail_collider.type = OBSTACLE;
    bottom_jail_collider.transparent = true;

    BoundingBox& bottom_jail_bb = registry.boundingBoxes.emplace(bottom_jail_bounds);
    bottom_jail_bb.width = scaled_tile_width * 12;
    bottom_jail_bb.height = scaled_tile_height/1.7f;

    base->addNonRenderedEntity(bottom_jail_bounds);

    // Visualize bounding box
    //base->addRenderedEntity(top_jail_bounds);
    // RenderRequest top_jail_request = {
    //     TEXTURE_ASSET_ID::TUNNEL,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // int top_jail_bound_id = top_jail_bounds;
    // base->entity_render_requests[top_jail_bound_id] = top_jail_request;

    Entity right_jail_wall_bounds = registry.create_entity();
    Motion& right_jail_wall_motion = registry.motions.emplace(right_jail_wall_bounds);

    right_jail_wall_motion.position = { scaled_tile_width / 6 + scaled_tile_width * 4 - scaled_tile_width / 5, scaled_tile_height / 2 + scaled_tile_height };
    right_jail_wall_motion.scale = { scaled_tile_width / 6, scaled_tile_height * 3 };
    right_jail_wall_motion.velocity = { 0, 0 };

    Collider& right_jail_wall_collider = registry.colliders.emplace(right_jail_wall_bounds);
    right_jail_wall_collider.type = OBSTACLE;
    right_jail_wall_collider.transparent = true;

    BoundingBox& right_jail_wall_bb = registry.boundingBoxes.emplace(right_jail_wall_bounds);
    right_jail_wall_bb.width = scaled_tile_width / 6;
    right_jail_wall_bb.height = scaled_tile_height * 3;

    base->addNonRenderedEntity(right_jail_wall_bounds);

    Entity top_right_edge_jail_wall_bounds = registry.create_entity();
    Motion& top_right_edge_jail_wall_motion = registry.motions.emplace(top_right_edge_jail_wall_bounds);

    top_right_edge_jail_wall_motion.position = {scaled_tile_width/6 + scaled_tile_width * 12 - scaled_tile_width/5, scaled_tile_height / 2 + scaled_tile_height};
    top_right_edge_jail_wall_motion.scale = {scaled_tile_width/6, scaled_tile_height*3};
    top_right_edge_jail_wall_motion.velocity = {0, 0};

    Collider& top_right_edge_jail_wall_collider = registry.colliders.emplace(top_right_edge_jail_wall_bounds);
    top_right_edge_jail_wall_collider.type = OBSTACLE;
    top_right_edge_jail_wall_collider.transparent = true;

    BoundingBox& top_right_edge_jail_wall_bb = registry.boundingBoxes.emplace(top_right_edge_jail_wall_bounds);
    top_right_edge_jail_wall_bb.width = scaled_tile_width/6;
    top_right_edge_jail_wall_bb.height = scaled_tile_height*3;

    base->addNonRenderedEntity(top_right_edge_jail_wall_bounds);

    Entity bottom_right_edge_jail_wall_bounds = registry.create_entity();
    Motion& bottom_right_edge_jail_wall_motion = registry.motions.emplace(bottom_right_edge_jail_wall_bounds);

    bottom_right_edge_jail_wall_motion.position = {scaled_tile_width/6 + scaled_tile_width * 12 - scaled_tile_width/5, scaled_tile_height / 2 + scaled_tile_height * 7};
    bottom_right_edge_jail_wall_motion.scale = {scaled_tile_width/6, scaled_tile_height*3};
    bottom_right_edge_jail_wall_motion.velocity = {0, 0};

    Collider& bottom_right_edge_jail_wall_collider = registry.colliders.emplace(bottom_right_edge_jail_wall_bounds);
    bottom_right_edge_jail_wall_collider.type = OBSTACLE;
    bottom_right_edge_jail_wall_collider.transparent = true;

    BoundingBox& bottom_right_edge_jail_wall_bb = registry.boundingBoxes.emplace(bottom_right_edge_jail_wall_bounds);
    bottom_right_edge_jail_wall_bb.width = scaled_tile_width/6;
    bottom_right_edge_jail_wall_bb.height = scaled_tile_height*3;

    base->addNonRenderedEntity(bottom_right_edge_jail_wall_bounds);

    // Visualize bounding box
    // base->addRenderedEntity(right_jail_wall_bounds);
    //     RenderRequest right_jail_request = {
    //     TEXTURE_ASSET_ID::SIDE_WALL_MID,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };

    // int right_jail_bound_id = right_jail_wall_bounds;
    // base->entity_render_requests[right_jail_bound_id] = right_jail_request;

    Entity left_jail_wall_bounds = registry.create_entity();
    Motion& left_jail_wall_motion = registry.motions.emplace(left_jail_wall_bounds);

    left_jail_wall_motion.position = { scaled_tile_width / 6 + scaled_tile_width * 2 + scaled_tile_width / 16, scaled_tile_height / 2 + scaled_tile_height };
    left_jail_wall_motion.scale = { scaled_tile_width / 6, scaled_tile_height * 3 };
    left_jail_wall_motion.velocity = { 0, 0 };

    Collider& left_jail_wall_collider = registry.colliders.emplace(left_jail_wall_bounds);
    left_jail_wall_collider.type = OBSTACLE;
    left_jail_wall_collider.transparent = true;

    BoundingBox& left_jail_wall_bb = registry.boundingBoxes.emplace(left_jail_wall_bounds);
    left_jail_wall_bb.width = scaled_tile_width / 6;
    left_jail_wall_bb.height = scaled_tile_height * 3;

    base->addNonRenderedEntity(left_jail_wall_bounds);

    // Visualize bounding box
    // base->addRenderedEntity(left_jail_wall_bounds);
    //     RenderRequest left_jail_request = {
    //     TEXTURE_ASSET_ID::SIDE_WALL_MID,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };

    // int left_jail_bound_id = left_jail_wall_bounds;
    // base->entity_render_requests[left_jail_bound_id] = left_jail_request;

    Entity back_wall_bounds = registry.create_entity();
    Motion& back_wall_motion = registry.motions.emplace(back_wall_bounds);

    back_wall_motion.position = { scaled_tile_width / 6 + 8 * scaled_tile_width, scaled_tile_height / 2 };
    back_wall_motion.scale = { scaled_tile_width * 16, scaled_tile_height };
    back_wall_motion.velocity = { 0, 0 };

    Collider& back_wall_collider = registry.colliders.emplace(back_wall_bounds);
    back_wall_collider.type = OBSTACLE;

    BoundingBox& back_wall_bb = registry.boundingBoxes.emplace(back_wall_bounds);
    back_wall_bb.width = 16 * scaled_tile_width;
    back_wall_bb.height = scaled_tile_height;

    base->addNonRenderedEntity(back_wall_bounds);

    Entity bottom_wall_bounds = registry.create_entity();
    Motion& bottom_wall_motion = registry.motions.emplace(bottom_wall_bounds);
    bottom_wall_motion.position = {scaled_tile_width / 6 + 8 * scaled_tile_width, scaled_tile_height * 9};
    bottom_wall_motion.scale = {scaled_tile_width * 16, scaled_tile_height};
    bottom_wall_motion.velocity = {0, 0};

    Collider& bottom_wall_collider = registry.colliders.emplace(bottom_wall_bounds);
    bottom_wall_collider.type = OBSTACLE;

    BoundingBox& bottom_wall_bb = registry.boundingBoxes.emplace(bottom_wall_bounds);
    bottom_wall_bb.width = 16 * scaled_tile_width;
    bottom_wall_bb.height = scaled_tile_height;

    base->addNonRenderedEntity(bottom_wall_bounds);

    Entity left_wall_bounds = registry.create_entity();
    Motion& left_wall_motion = registry.motions.emplace(left_wall_bounds);
    left_wall_motion.position = {0, scaled_tile_height * 4.5f};
    left_wall_motion.scale = {scaled_tile_width, scaled_tile_height*9.f};
    left_wall_motion.velocity = {0, 0};

    Collider& left_wall_collider = registry.colliders.emplace(left_wall_bounds);
    left_wall_collider.type = OBSTACLE;

    BoundingBox& left_wall_bb = registry.boundingBoxes.emplace(left_wall_bounds);
    left_wall_bb.width = scaled_tile_width;
    left_wall_bb.height = scaled_tile_height * 9.f;

    base->addNonRenderedEntity(left_wall_bounds);

    Entity right_wall_bounds = registry.create_entity();
    Motion& right_wall_motion = registry.motions.emplace(right_wall_bounds);
    right_wall_motion.position = {scaled_tile_width*16.33f, scaled_tile_height * 4.5f};
    right_wall_motion.scale = {scaled_tile_width, scaled_tile_height*9.f};
    right_wall_motion.velocity = {0, 0};

    Collider& right_wall_collider = registry.colliders.emplace(right_wall_bounds);
    right_wall_collider.type = OBSTACLE;

    BoundingBox& right_wall_bb = registry.boundingBoxes.emplace(right_wall_bounds);
    right_wall_bb.width = scaled_tile_width;
    right_wall_bb.height = scaled_tile_height * 9.f;

    base->addNonRenderedEntity(right_wall_bounds);

    Entity patrol = createPatrol({scaled_tile_width*6, scaled_tile_height*4.4}, 0, scaled_tile_width, scaled_tile_height);
    registry.colliders.get(patrol).type = CREATURE;
    addPatrolToRoom(renderer, base, patrol);
    registry.randomWalkers.remove(patrol);
    registry.setMotions.emplace(patrol);
    registry.motions.remove(registry.patrols.get(patrol).light);

    base->room_height = window_height;
    base->room_width = window_width;
    level->addRoom(base);
}

void createTunnelRoom(RenderSystem* renderer, Level* level) {
    Room* tunnel = new Room("tunnel room");
    tunnel->gridDimensions = {26, 10};

    Entity tunnel_player = registry.players.entities[0];
    int tunnel_player_id = tunnel_player;

    tile_width = 32.0f;
    tile_height = 32.0f;
    grid_columns = 15;
    grid_rows = 13;

    scale_x = window_width / (tile_width * grid_columns);
    scale_y = window_height / (tile_height * grid_rows);
    room_scale = std::min(scale_x, scale_y);

    scaled_tile_width = tile_width * room_scale;
    scaled_tile_height = tile_height * room_scale;

    offset_x = (window_width - (scaled_tile_width * grid_columns)) / 2.0f;
    offset_y = (window_height - (scaled_tile_height * grid_rows)) / 2.0f;

    tunnel->player_position = { 2.5 * scaled_tile_width + scaled_tile_width / 2 + offset_x, 8 * scaled_tile_height };
    level->originalSpawnPositions.push_back(tunnel->player_position);
    tunnel->player_scale = { scaled_tile_width * 1.5, scaled_tile_height * 1.5 };

    // Tunnel left wall
    for (int row = 5; row < 8; row++) {
        Entity tunnel_left = registry.create_entity();
        Motion& tunnel_left_motion = registry.motions.emplace(tunnel_left);

        tunnel_left_motion.position = { scaled_tile_width / 2 + offset_x, row * scaled_tile_height + scaled_tile_height / 2 };
        tunnel_left_motion.scale = { scaled_tile_width, scaled_tile_height };
        tunnel_left_motion.velocity = { 0, 0 };

        tunnel->addRenderedEntity(tunnel_left);

        RenderRequest tunnel_left_request = {
            TEXTURE_ASSET_ID::TUNNEL,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
            {},
            false
        };

        int tunnel_left_id = tunnel_left;
        tunnel->entity_render_requests[tunnel_left_id] = tunnel_left_request;
    }

    // Tunnel top wall
    for (int col = 1; col < grid_columns - 1; col++) {
        Entity tunnel_wall = registry.create_entity();
        Motion& tunnel_wall_motion = registry.motions.emplace(tunnel_wall);

        tunnel_wall_motion.position = { col * scaled_tile_width + scaled_tile_width / 2 + offset_x, 5 * scaled_tile_height };
        tunnel_wall_motion.scale = { scaled_tile_width, scaled_tile_height };
        tunnel_wall_motion.velocity = { 0, 0 };

        tunnel->addRenderedEntity(tunnel_wall);

        RenderRequest tunnel_wall_request = {
            TEXTURE_ASSET_ID::TUNNEL_WALL,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
            {},
            false
        };

        int tunnel_wall_id = tunnel_wall;
        tunnel->entity_render_requests[tunnel_wall_id] = tunnel_wall_request;
    }

    // Tunnel right wall
    for (int row = 5; row < 8; row++) {
        if (row == 6) {
            Entity tunnel_floor = registry.create_entity();
            Motion& tunnel_floor_motion = registry.motions.emplace(tunnel_floor);

            tunnel_floor_motion.position = { 14 * scaled_tile_width + scaled_tile_width / 2 + offset_x, row * scaled_tile_height + scaled_tile_height / 2 };
            tunnel_floor_motion.scale = { scaled_tile_width, scaled_tile_height };
            tunnel_floor_motion.velocity = { 0, 0 };

            tunnel->addRenderedEntity(tunnel_floor);

            int random_number = rand() % 10 + 1; // Generate a random number between 1 and 10
            TEXTURE_ASSET_ID texture_id;

            if (random_number <= 6) {
                texture_id = TEXTURE_ASSET_ID::TUNNEL_FLOOR_2;
            }
            else if (random_number <= 8) {
                texture_id = TEXTURE_ASSET_ID::TUNNEL_FLOOR_1;
            }
            else {
                texture_id = TEXTURE_ASSET_ID::TUNNEL_FLOOR_3;
            }

            RenderRequest tunnel_floor_request = {
                texture_id,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                {},
                false
            };

            int tunnel_floor_id = tunnel_floor;
            tunnel->entity_render_requests[tunnel_floor_id] = tunnel_floor_request;
        }
        else {
            Entity tunnel_right = registry.create_entity();
            Motion& tunnel_right_motion = registry.motions.emplace(tunnel_right);

            tunnel_right_motion.position = { 14 * scaled_tile_width + scaled_tile_width / 2.0f + offset_x, row * scaled_tile_height + scaled_tile_height / 2.0f };
            tunnel_right_motion.scale = { scaled_tile_width, scaled_tile_height };
            tunnel_right_motion.velocity = { 0, 0 };

            tunnel->addRenderedEntity(tunnel_right);

            RenderRequest tunnel_right_request = {
                TEXTURE_ASSET_ID::TUNNEL,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                {},
                false
            };

            int tunnel_right_id = tunnel_right;
            tunnel->entity_render_requests[tunnel_right_id] = tunnel_right_request;
        }
    }

    // Tunnel floor
    for (int row = 6; row < 8; row++) {
        for (int col = 1; col < grid_columns - 1; col++) {
            Entity tunnel_floor = registry.create_entity();
            Motion& tunnel_floor_motion = registry.motions.emplace(tunnel_floor);

            tunnel_floor_motion.position = { col * scaled_tile_width + scaled_tile_width / 2 + offset_x, row * scaled_tile_height };
            tunnel_floor_motion.scale = { scaled_tile_width, scaled_tile_height };
            tunnel_floor_motion.velocity = { 0, 0 };

            tunnel->addRenderedEntity(tunnel_floor);

            int random_number = rand() % 10 + 1; // Generate a random number between 1 and 10
            TEXTURE_ASSET_ID texture_id;

            if (random_number <= 6) {
                texture_id = TEXTURE_ASSET_ID::TUNNEL_FLOOR_2;
            }
            else if (random_number <= 8) {
                texture_id = TEXTURE_ASSET_ID::TUNNEL_FLOOR_1;
            }
            else {
                texture_id = TEXTURE_ASSET_ID::TUNNEL_FLOOR_3;
            }

            RenderRequest tunnel_floor_request = {
                texture_id,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                {},
                false
            };

            int tunnel_floor_id = tunnel_floor;
            tunnel->entity_render_requests[tunnel_floor_id] = tunnel_floor_request;
        }
    }

    // Tunnel entrance
    for (int col = 2; col < 4; col++) {
        Entity tunnel_entrance = registry.create_entity();
        Motion& tunnel_entrance_motion = registry.motions.emplace(tunnel_entrance);

        tunnel_entrance_motion.position = { col * scaled_tile_width + scaled_tile_width / 2 + offset_x, 8 * scaled_tile_height };
        tunnel_entrance_motion.scale = { scaled_tile_width, scaled_tile_height };
        tunnel_entrance_motion.velocity = { 0, 0 };

        tunnel->addRenderedEntity(tunnel_entrance);

        TEXTURE_ASSET_ID texture_id;
        if (col == 2) {
            texture_id = TEXTURE_ASSET_ID::TUNNEL_FLOOR_1;
        }
        else {
            texture_id = TEXTURE_ASSET_ID::TUNNEL_FLOOR_3;
        }

        RenderRequest tunnel_entrance_request = {
            texture_id,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE,
            {},
            false
        };

        int tunnel_entrance_id = tunnel_entrance;
        tunnel->entity_render_requests[tunnel_entrance_id] = tunnel_entrance_request;

    }

    tunnel->addRenderedEntity(tunnel_player);
    tunnel->addNonRenderedEntity(tunnel_player);

    RenderRequest player_request_2;
    player_request_2.used_texture = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    player_request_2.used_effect = EFFECT_ASSET_ID::TEXTURED;
    player_request_2.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    player_request_2.hasAnimation = true;
    renderer->setAnimation(player_request_2, AnimationState::MOVING_UP, renderer->catAnimationMap);
    tunnel->entity_render_requests[tunnel_player_id] = player_request_2;

    // Tunnel bottom
    for (int col = 1; col < grid_columns - 1; col++) {
        if (col != 2 && col != 3) {
            Entity tunnel_bottom = registry.create_entity();
            Motion& tunnel_bottom_motion = registry.motions.emplace(tunnel_bottom);

            tunnel_bottom_motion.position = { col * scaled_tile_width + scaled_tile_width / 2 + offset_x, 8 * scaled_tile_height };
            tunnel_bottom_motion.scale = { scaled_tile_width, scaled_tile_height };
            tunnel_bottom_motion.velocity = { 0, 0 };

            tunnel->addRenderedEntity(tunnel_bottom);

            RenderRequest tunnel_right_request = {
                TEXTURE_ASSET_ID::TUNNEL,
                EFFECT_ASSET_ID::TEXTURED,
                GEOMETRY_BUFFER_ID::SPRITE,
                {},
                false
            };

            int tunnel_bottom_id = tunnel_bottom;
            tunnel->entity_render_requests[tunnel_bottom_id] = tunnel_right_request;
        }
    }

    // tunnel exit
    Entity tunnel_exit = registry.create_entity();
    Motion& tunnel_exit_motion = registry.motions.emplace(tunnel_exit);
    registry.doors.emplace(tunnel_exit);

    tunnel_exit_motion.position = { 2.5 * scaled_tile_width + scaled_tile_width / 2 + offset_x, 9 * scaled_tile_height };
    tunnel_exit_motion.scale = { scaled_tile_width * 2, scaled_tile_height };
    tunnel_exit_motion.velocity = { 0, 0 };

    Collider& tunnel_exit_collider = registry.colliders.emplace(tunnel_exit);
    tunnel_exit_collider.type = DOOR;

    BoundingBox& tunnel_exit_bb = registry.boundingBoxes.emplace(tunnel_exit);
    tunnel_exit_bb.width = scaled_tile_width * 2;
    tunnel_exit_bb.height = scaled_tile_height;

    tunnel->addNonRenderedEntity(tunnel_exit);

    tunnel_to_base_id = tunnel_exit;

    level->spawnPositions[tunnel_to_base_id] = { 2.5 * scaled_tile_width + scaled_tile_width / 2 + offset_x, 8 * scaled_tile_height };

    // Bounding boxes
    Entity tunnel_top_bounds = registry.create_entity();

    Motion& tunnel_top_motion = registry.motions.emplace(tunnel_top_bounds);
    tunnel_top_motion.position = { offset_x + scaled_tile_width / 2 + 7 * scaled_tile_width, 4.8 * scaled_tile_height };
    tunnel_top_motion.scale = { scaled_tile_width * 12.8, scaled_tile_height };
    tunnel_top_motion.velocity = { 0, 0 };

    Collider& tunnel_top_collider = registry.colliders.emplace(tunnel_top_bounds);
    tunnel_top_collider.type = OBSTACLE;

    BoundingBox& tunnel_top_bb = registry.boundingBoxes.emplace(tunnel_top_bounds);
    tunnel_top_bb.width = scaled_tile_width * 12.8f;
    tunnel_top_bb.height = scaled_tile_height;

    tunnel->addNonRenderedEntity(tunnel_top_bounds);

    // Visualize bounding box
    // tunnel->addRenderedEntity(tunnel_top_bounds);

    // RenderRequest tunnel_top_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // int tunnel_top_id = tunnel_top_bounds;
    // tunnel->entity_render_requests[tunnel_top_id] = tunnel_top_request;

    Entity tunnel_left_bounds = registry.create_entity();

    Motion& tunnel_left_motion = registry.motions.emplace(tunnel_left_bounds);
    tunnel_left_motion.position = { 0.8 * scaled_tile_width + offset_x, 6 * scaled_tile_height + scaled_tile_height / 2 };
    tunnel_left_motion.scale = { scaled_tile_width, scaled_tile_height * 3 };
    tunnel_left_motion.velocity = { 0, 0 };

    Collider& tunnel_left_collider = registry.colliders.emplace(tunnel_left_bounds);
    tunnel_left_collider.type = OBSTACLE;

    BoundingBox& tunnel_left_bb = registry.boundingBoxes.emplace(tunnel_left_bounds);
    tunnel_left_bb.width = scaled_tile_width;
    tunnel_left_bb.height = scaled_tile_height * 3;

    tunnel->addNonRenderedEntity(tunnel_left_bounds);

    // Visualize bounding box
    // tunnel->addRenderedEntity(tunnel_left_bounds);

    // RenderRequest tunnel_left_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };

    // int tunnel_left_id = tunnel_left_bounds;
    // tunnel->entity_render_requests[tunnel_left_id] = tunnel_left_request;

    Entity tunnel_bl_bounds = registry.create_entity();

    Motion& tunnel_bl_motion = registry.motions.emplace(tunnel_bl_bounds);
    tunnel_bl_motion.position = { scaled_tile_width + scaled_tile_width / 2 + offset_x, 8 * scaled_tile_height };
    tunnel_bl_motion.scale = { scaled_tile_width * 1.7, scaled_tile_height * 1.6 };
    tunnel_bl_motion.velocity = { 0, 0 };

    Collider& tunnel_bl_collider = registry.colliders.emplace(tunnel_bl_bounds);
    tunnel_bl_collider.type = OBSTACLE;

    BoundingBox& tunnel_bl_bb = registry.boundingBoxes.emplace(tunnel_bl_bounds);
    tunnel_bl_bb.width = scaled_tile_width * 1.7f;
    tunnel_bl_bb.height = scaled_tile_height * 1.6f;

    tunnel->addNonRenderedEntity(tunnel_bl_bounds);

    // Visualize bounding box
    // tunnel->addRenderedEntity(tunnel_bl_bounds);
    // RenderRequest tunnel_bl_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // int tunnel_bl_id = tunnel_bl_bounds;
    // tunnel->entity_render_requests[tunnel_bl_id] = tunnel_bl_request;

    Entity tunnel_br_bounds = registry.create_entity();

    Motion& tunnel_br_motion = registry.motions.emplace(tunnel_br_bounds);
    tunnel_br_motion.position = { 8.5 * scaled_tile_width + scaled_tile_width / 2 + offset_x, 8 * scaled_tile_height };
    tunnel_br_motion.scale = { scaled_tile_width * 10.5, scaled_tile_height * 1.6 };
    tunnel_br_motion.velocity = { 0, 0 };

    Collider& tunnel_br_collider = registry.colliders.emplace(tunnel_br_bounds);
    tunnel_br_collider.type = OBSTACLE;

    BoundingBox& tunnel_br_bb = registry.boundingBoxes.emplace(tunnel_br_bounds);
    tunnel_br_bb.width = scaled_tile_width * 10.5f;
    tunnel_br_bb.height = scaled_tile_height * 1.6f;

    tunnel->addNonRenderedEntity(tunnel_br_bounds);

    // Visualize bounding box
    // tunnel->addRenderedEntity(tunnel_br_bounds);
    // RenderRequest tunnel_br_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // int tunnel_br_id = tunnel_br_bounds;
    // tunnel->entity_render_requests[tunnel_br_id] = tunnel_br_request;

    Entity tunnel_rt_bounds = registry.create_entity();
    Motion& tunnel_rt_motion = registry.motions.emplace(tunnel_rt_bounds);
    tunnel_rt_motion.position = { 13.8 * scaled_tile_width + scaled_tile_width / 2 + offset_x, 5 * scaled_tile_height + scaled_tile_height / 2 };
    tunnel_rt_motion.scale = { scaled_tile_width * 1.4, scaled_tile_height };
    tunnel_rt_motion.velocity = { 0, 0 };

    Collider& tunnel_rt_collider = registry.colliders.emplace(tunnel_rt_bounds);
    tunnel_rt_collider.type = OBSTACLE;

    BoundingBox& tunnel_rt_bb = registry.boundingBoxes.emplace(tunnel_rt_bounds);
    tunnel_rt_bb.width = scaled_tile_width * 1.4f;
    tunnel_rt_bb.height = scaled_tile_height;

    tunnel->addNonRenderedEntity(tunnel_rt_bounds);

    vec2 backpack_pos = vec2{ scaled_tile_width * 16, scaled_tile_height * 6 };

    std::pair<Entity, Entity> backpack = createBackpack(renderer, backpack_pos);
    Entity backpackMesh = backpack.first;
    Entity backpackTexture = backpack.second;

    Motion& dummyMotion = registry.motions.get(backpackTexture);
    dummyMotion.scale = { scaled_tile_width, scaled_tile_height }; // make the texture a bit bigger

    tunnel->addRenderedEntity(backpackTexture); // Get the texture from the png
    tunnel->addRenderedEntity(backpackMesh); // Get the texture from the png
    tunnel->addNonRenderedEntity(backpackMesh); // Add the mesh entity for collision

    RenderRequest backpack_render_request =
    { TEXTURE_ASSET_ID::TEXTURE_COUNT,
    EFFECT_ASSET_ID::BACKPACK,
    GEOMETRY_BUFFER_ID::BACKPACK,
    {},
    false };

    RenderRequest texture_render_request = {
        TEXTURE_ASSET_ID::BROWN_BAG,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };
    tunnel->entity_render_requests[backpackTexture] = texture_render_request;
    tunnel->entity_render_requests[backpackMesh] = backpack_render_request;
    tunnel->meshToTextureMap[backpackMesh] = backpackTexture;
    // Visualize bounding box
    // tunnel->addRenderedEntity(tunnel_rt_bounds);
    // RenderRequest tunnel_rt_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // int tunnel_rt_id = tunnel_rt_bounds;
    // tunnel->entity_render_requests[tunnel_rt_id] = tunnel_rt_request;

    Entity tunnel_rb_bounds = registry.create_entity();
    Motion& tunnel_rb_motion = registry.motions.emplace(tunnel_rb_bounds);
    tunnel_rb_motion.position = { 13.8 * scaled_tile_width + scaled_tile_width / 2 + offset_x, 6.7 * scaled_tile_height + scaled_tile_height / 2 };
    tunnel_rb_motion.scale = { scaled_tile_width * 1.4, scaled_tile_height };
    tunnel_rb_motion.velocity = { 0, 0 };

    Collider& tunnel_rb_collider = registry.colliders.emplace(tunnel_rb_bounds);
    tunnel_rb_collider.type = OBSTACLE;

    BoundingBox& tunnel_rb_bb = registry.boundingBoxes.emplace(tunnel_rb_bounds);
    tunnel_rb_bb.width = scaled_tile_width * 1.4f;
    tunnel_rb_bb.height = scaled_tile_height;

    tunnel->addNonRenderedEntity(tunnel_rb_bounds);

    // Visualize bounding box
    // tunnel->addRenderedEntity(tunnel_rb_bounds);
    // RenderRequest tunnel_rb_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // int tunnel_rb_id = tunnel_rb_bounds;
    // tunnel->entity_render_requests[tunnel_rb_id] = tunnel_rb_request;

    Entity tunnel_map_exit = registry.create_entity();
    Motion& tunnel_map_motion = registry.motions.emplace(tunnel_map_exit);
    Door& tunnel_map_door = registry.doors.emplace(tunnel_map_exit);

    tunnel_map_motion.position = { 14.5 * scaled_tile_width + scaled_tile_width / 2 + offset_x, 6 * scaled_tile_height + scaled_tile_height / 2 };
    tunnel_map_motion.scale = { scaled_tile_width * 0.7, scaled_tile_height };
    tunnel_map_motion.velocity = { 0, 0 };

    Collider& tunnel_map_collider = registry.colliders.emplace(tunnel_map_exit);
    tunnel_map_collider.type = DOOR;

    BoundingBox& tunnel_map_bb = registry.boundingBoxes.emplace(tunnel_map_exit);
    tunnel_map_bb.width = scaled_tile_width * 0.7;
    tunnel_map_bb.height = scaled_tile_height;

    tunnel->addNonRenderedEntity(tunnel_map_exit);
    tunnel_to_map_id = tunnel_map_exit;

    level->spawnPositions[tunnel_to_map_id] = {200, 3950};

    Stats fishStats;
    fishStats.currentHp = -10;
    Entity fish = createConsumable({ scaled_tile_width * 10,scaled_tile_height * 6 }, fishStats, TEXTURE_ASSET_ID::BLUE_FISH);
    tunnel->addRenderedEntity(fish);

    RenderRequest fish_render_request = {
        TEXTURE_ASSET_ID::BLUE_FISH,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    tunnel->entity_render_requests[fish] = fish_render_request;

    Stats flashlightStats;
    flashlightStats.agility = 5;
    Entity flashlight = createEquippable({ scaled_tile_width * 7,scaled_tile_height * 6 }, flashlightStats, TEXTURE_ASSET_ID::FLASH_LIGHT);
    tunnel->addRenderedEntity(flashlight);

    RenderRequest flashlight_render_request = {
        TEXTURE_ASSET_ID::FLASH_LIGHT,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    tunnel->entity_render_requests[flashlight] = flashlight_render_request;


    // BOUNCER NPC ENTITY
    Entity bouncer = registry.create_entity();
    Motion& bouncer_motion = registry.motions.emplace(bouncer);
    // bouncer_motion.position = {centered_x + (scaled_tile_width * 15.6), centered_y + (scaled_tile_height *6.2)};
    bouncer_motion.position = {14 * scaled_tile_width + scaled_tile_width / 2.0f + offset_x, 6 * scaled_tile_height + scaled_tile_height / 3.0f};
    bouncer_motion.scale = {scaled_tile_width * 1.8, scaled_tile_height * 1.8};
    bouncer_motion.velocity = {0, 0};

    Collider& bouncer_collider = registry.colliders.emplace(bouncer);
    bouncer_collider.type = CREATURE;

    BoundingBox& bouncer_bb = registry.boundingBoxes.emplace(bouncer);
    bouncer_bb.width = scaled_tile_width;
    bouncer_bb.height = scaled_tile_height;

    Stats& bouncer_stats = registry.stats.emplace(bouncer);
    bouncer_stats.maxHp = 15;
    bouncer_stats.currentHp = 15;
    bouncer_stats.reputation = 10;
    bouncer_stats.cuteness = 20;
    bouncer_stats.ferocity = 1;

    NPC& bouncer_npc = registry.npcs.emplace(bouncer);
    bouncer_npc.encounter_texture_id = (int)TEXTURE_ASSET_ID::RACOON_ATTACK;
    bouncer_npc.name = "Raccoon";
    bouncer_npc.isInteractable = true;
    bouncer_npc.isTutorialNPC = true;

    bouncer_npc.attackDuration = 10.0f;

    bouncer_npc.dialogue.push("Raccoon: Before I let you out, you have to learn how to battle first");

    Animation animation;
    animation.frameCount = 1;
    animation.currentFrame = 0;
    animation.frameTime = 0.0f;
    animation.elapsedTime = 0.0f;
    animation.columns = 4;
    animation.rows = 13;
    animation.startRow= 2;
    animation.startCol = 0;

    tunnel->addNonRenderedEntity(bouncer);
    tunnel->addRenderedEntity(bouncer);
    RenderRequest bouncer_request = {
        TEXTURE_ASSET_ID::RACCOON,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        animation,
        true
    };
    int bouncer_id = bouncer;
    tunnel->entity_render_requests[bouncer_id] = bouncer_request;

    Entity raccoon_dialogue_icon = registry.create_entity();
    Motion& raccoon_icon_motion = registry.motions.emplace(raccoon_dialogue_icon);

    raccoon_icon_motion.position = {bouncer_motion.position.x, bouncer_motion.position.y - scaled_tile_height*0.7};
    raccoon_icon_motion.scale = {scaled_tile_width * 0.6, scaled_tile_height * 0.6};
    raccoon_icon_motion.velocity = {0, 0};

    registry.uiElements.emplace(raccoon_dialogue_icon);

    tunnel->addRenderedEntity(raccoon_dialogue_icon);
    RenderRequest raccoon_dialogue_request = {
        TEXTURE_ASSET_ID::DIALOGUE_ICON,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    bouncer_npc.interactIcon = raccoon_dialogue_icon;

    int raccoon_dialogue_icon_id = raccoon_dialogue_icon;
    tunnel->entity_render_requests[raccoon_dialogue_icon_id] = raccoon_dialogue_request;

    tunnel->room_height = window_height;
    tunnel->room_width = window_width;
    level->addRoom(tunnel);
}

void createLobbyRoom(Level* level, RenderSystem* renderer) {
    Room* lobby = new Room("lobby room");
    lobby->gridDimensions = {20, 12};

    Entity lobby_player = registry.players.entities[0];
    int lobby_player_id = lobby_player;

    tile_width = 48.0f;
    tile_height = 48.0f;
    grid_columns = 20;
    grid_rows = 12;

    scale_x = window_width / (tile_width * grid_columns);
    scale_y = window_height / (tile_height * grid_rows);
    room_scale = std::min(scale_x, scale_y);

    scaled_tile_width = tile_width * room_scale;
    scaled_tile_height = tile_height * room_scale;

    float scale_map_x = window_width / 960.0f;
    float scale_map_y = window_height / 576.0f;
    float scale_map = std::min(scale_map_x, scale_map_y); // Maintain aspect ratio

    float centered_x = (window_width - (960.0f * scale_map)) / 2;
    float centered_y = (window_height - (576.0f * scale_map)) / 2;

    float scaled_tilemap_width = 960.0f * scale_map;
    float scaled_tilemap_height = 576.0f * scale_map;

    float position_x = (window_width - scaled_tilemap_width) / 2;
    float position_y = (window_height - scaled_tilemap_height) / 2;

    Entity lobby_tilemap = registry.create_entity();
    Motion& tilemap_motion = registry.motions.emplace(lobby_tilemap);

    tilemap_motion.position = { position_x + scaled_tilemap_width / 2, position_y + scaled_tilemap_height / 2 };
    tilemap_motion.scale = { scaled_tilemap_width, scaled_tilemap_height };
    tilemap_motion.velocity = { 0, 0 };

    lobby->addRenderedEntity(lobby_tilemap);

    RenderRequest tilemap_request = {
        TEXTURE_ASSET_ID::LOBBY_TILE_MAP,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    int tilemap_id = lobby_tilemap;
    lobby->entity_render_requests[tilemap_id] = tilemap_request;

    Entity lobby_tilemap_behind = registry.create_entity();
    Motion& tilemap_behind_motion = registry.motions.emplace(lobby_tilemap_behind);

    tilemap_behind_motion.position = { position_x + scaled_tilemap_width / 2, position_y + scaled_tilemap_height / 2 };
    tilemap_behind_motion.scale = { scaled_tilemap_width, scaled_tilemap_height };
    tilemap_behind_motion.velocity = { 0, 0 };

    lobby->addRenderedEntity(lobby_tilemap_behind);

    RenderRequest tilemap_behind_request = {
        TEXTURE_ASSET_ID::LOBBY_BEHIND_ASSETS,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    int tilemap_behind_id = lobby_tilemap_behind;
    lobby->entity_render_requests[tilemap_behind_id] = tilemap_behind_request;

    lobby->player_position = {
        centered_x + (scaled_tile_width * 1.5),
        centered_y + (scaled_tile_height * 2.4)
    };
    lobby->player_scale = { scaled_tile_width * 1.2, scaled_tile_height * 1.2 };

    lobby->addRenderedEntity(lobby_player);
    lobby->addNonRenderedEntity(lobby_player);

    RenderRequest player_request_3;
    player_request_3.used_texture = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    player_request_3.used_effect = EFFECT_ASSET_ID::TEXTURED;
    player_request_3.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    player_request_3.hasAnimation = true;
    renderer->setAnimation(player_request_3, AnimationState::MOVING_RIGHT, renderer->catAnimationMap);
    lobby->entity_render_requests[lobby_player_id] = player_request_3;

    // // Bounding boxes

    //level->spawnPositions[lobby_to_tunnel_id] = { centered_x + (scaled_tile_width * 1.5),
    // centered_y + (scaled_tile_height * 2.4) };

    Entity lobby_map_exit = registry.create_entity();
    Motion& lobby_map_motion = registry.motions.emplace(lobby_map_exit);
    registry.doors.emplace(lobby_map_exit);

    // lobby_map_motion.position = {centered_x + (scaled_tile_width * 18.6), centered_y + (scaled_tile_height *6.2)}; keeping it for future reference
    lobby_map_motion.position = { centered_x + (scaled_tile_width / 4), centered_y + (scaled_tile_height * 2.5)};
    lobby_map_motion.scale = { scaled_tile_width, scaled_tile_height };
    lobby_map_motion.velocity = { 0, 0 };

    Collider& lobby_map_collider = registry.colliders.emplace(lobby_map_exit);
    lobby_map_collider.type = DOOR;

    BoundingBox& lobby_map_bb = registry.boundingBoxes.emplace(lobby_map_exit);
    lobby_map_bb.width = scaled_tile_width;
    lobby_map_bb.height = scaled_tile_height;

    lobby->addNonRenderedEntity(lobby_map_exit);
    lobby_to_map_id = lobby_map_exit;

    level->spawnPositions[lobby_to_map_id] = { centered_x + (scaled_tile_width * 1.5), centered_y + (scaled_tile_height * 2.4)};

    // Visualize exit
    // lobby->addRenderedEntity(lobby_tunnel_exit);
    // RenderRequest lobby_tunnel_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // lobby->entity_render_requests[lobby_to_tunnel_id] = lobby_tunnel_request;

    Entity lobby_top_bounds = registry.create_entity();
    Motion& lobby_top_motion = registry.motions.emplace(lobby_top_bounds);
    lobby_top_motion.position = { centered_x + (scaled_tile_width * 10), centered_y + (scaled_tile_height * 1.8) };
    lobby_top_motion.scale = { scaled_tile_width * 20, scaled_tile_height / 3 };
    lobby_top_motion.velocity = { 0, 0 };

    Collider& lobby_top_collider = registry.colliders.emplace(lobby_top_bounds);
    lobby_top_collider.type = OBSTACLE;

    BoundingBox& lobby_top_bb = registry.boundingBoxes.emplace(lobby_top_bounds);
    lobby_top_bb.width = scaled_tile_width * 20;
    lobby_top_bb.height = scaled_tile_height / 3;

    lobby->addNonRenderedEntity(lobby_top_bounds);

    // Visualize bounds
    // lobby->addRenderedEntity(lobby_top_bounds);
    // RenderRequest lobby_top_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };

    // int lobby_top_id = lobby_top_bounds;
    // lobby->entity_render_requests[lobby_top_id] = lobby_top_request;

    Entity lobby_left_bounds = registry.create_entity();
    Motion& lobby_left_motion = registry.motions.emplace(lobby_left_bounds);
    lobby_left_motion.position = { centered_x + (scaled_tile_width / 1.5), centered_y + (scaled_tile_height * 7.1) };
    lobby_left_motion.scale = { scaled_tile_width, scaled_tile_height * 8.2 };
    lobby_left_motion.velocity = { 0, 0 };

    Collider& lobby_left_collider = registry.colliders.emplace(lobby_left_bounds);
    lobby_left_collider.type = OBSTACLE;

    BoundingBox& lobby_left_bb = registry.boundingBoxes.emplace(lobby_left_bounds);
    lobby_left_bb.width = scaled_tile_width;
    lobby_left_bb.height = scaled_tile_height * 8.2f;

    lobby->addNonRenderedEntity(lobby_left_bounds);

    // Visualize bounding box
    // lobby->addRenderedEntity(lobby_left_bounds);
    // RenderRequest lobby_left_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // int lobby_left_id = lobby_left_bounds;
    // lobby->entity_render_requests[lobby_left_id] = lobby_left_request;

    Entity lobby_bottom_bounds = registry.create_entity();
    Motion& lobby_bottom_motion = registry.motions.emplace(lobby_bottom_bounds);
    lobby_bottom_motion.position = { centered_x + (scaled_tile_width * 10), centered_y + (scaled_tile_height * 10.8) };
    lobby_bottom_motion.scale = { scaled_tile_width * 20, scaled_tile_height / 3 };
    lobby_bottom_motion.velocity = { 0, 0 };

    Collider& lobby_bottom_collider = registry.colliders.emplace(lobby_bottom_bounds);
    lobby_bottom_collider.type = OBSTACLE;

    BoundingBox& lobby_bottom_bb = registry.boundingBoxes.emplace(lobby_bottom_bounds);
    lobby_bottom_bb.width = scaled_tile_width * 20;
    lobby_bottom_bb.height = scaled_tile_height / 3;

    lobby->addNonRenderedEntity(lobby_bottom_bounds);

    // Visualize bounds
    // lobby->addRenderedEntity(lobby_bottom_bounds);
    // RenderRequest lobby_bottom_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };

    // int lobby_bottom_id = lobby_bottom_bounds;
    // lobby->entity_render_requests[lobby_bottom_id] = lobby_bottom_request;

    Entity lobby_shelf_bounds = registry.create_entity();
    Motion& lobby_shelf_motion = registry.motions.emplace(lobby_shelf_bounds);
    lobby_shelf_motion.position = { centered_x + (scaled_tile_width * 5), centered_y + (scaled_tile_height * 1.7) };
    lobby_shelf_motion.scale = { scaled_tile_width * 2.5, scaled_tile_height };
    lobby_shelf_motion.velocity = { 0, 0 };

    Collider& lobby_shelf_collider = registry.colliders.emplace(lobby_shelf_bounds);
    lobby_shelf_collider.type = OBSTACLE;
    lobby_shelf_collider.transparent = true;

    BoundingBox& lobby_shelf_bb = registry.boundingBoxes.emplace(lobby_shelf_bounds);
    lobby_shelf_bb.width = scaled_tile_width * 2.5f;
    lobby_shelf_bb.height = scaled_tile_height;

    lobby->addNonRenderedEntity(lobby_shelf_bounds);

    // Visualize bounds
    // lobby->addRenderedEntity(lobby_shelf_bounds);
    // RenderRequest lobby_shelf_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };

    // int lobby_shelf_id = lobby_shelf_bounds;
    // lobby->entity_render_requests[lobby_shelf_id] = lobby_shelf_request;

    Entity lobby_plants_bounds = registry.create_entity();
    Motion& lobby_plants_motion = registry.motions.emplace(lobby_plants_bounds);
    lobby_plants_motion.position = { centered_x + (scaled_tile_width * 9.4), centered_y + (scaled_tile_height * 2) };
    lobby_plants_motion.scale = { scaled_tile_width * 7.3, scaled_tile_height };
    lobby_plants_motion.velocity = { 0, 0 };

    Collider& lobby_plants_collider = registry.colliders.emplace(lobby_plants_bounds);
    lobby_plants_collider.type = OBSTACLE;
    lobby_plants_collider.transparent = true;

    BoundingBox& lobby_plants_bb = registry.boundingBoxes.emplace(lobby_plants_bounds);
    lobby_plants_bb.width = scaled_tile_width * 7.3f;
    lobby_plants_bb.height = scaled_tile_height;

    lobby->addNonRenderedEntity(lobby_plants_bounds);

    // Visualize bounding box
    // lobby->addRenderedEntity(lobby_plants_bounds);
    // RenderRequest lobby_plants_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // int lobby_plants_id = lobby_plants_bounds;
    // lobby->entity_render_requests[lobby_plants_id] = lobby_plants_request;

    Entity lobby_desk_bounds = registry.create_entity();
    Motion& lobby_desk_motion = registry.motions.emplace(lobby_desk_bounds);
    lobby_desk_motion.position = { centered_x + (scaled_tile_width * 9.5), centered_y + (scaled_tile_height * 2.8) };
    lobby_desk_motion.scale = { scaled_tile_width * 5.4, scaled_tile_height * 4 };
    lobby_desk_motion.velocity = { 0, 0 };

    Collider& lobby_desk_collider = registry.colliders.emplace(lobby_desk_bounds);
    lobby_desk_collider.type = OBSTACLE;

    BoundingBox& lobby_desk_bb = registry.boundingBoxes.emplace(lobby_desk_bounds);
    lobby_desk_bb.width = scaled_tile_width * 5.4f;
    lobby_desk_bb.height = scaled_tile_height * 4;

    lobby->addNonRenderedEntity(lobby_desk_bounds);

    // Visualize bounding box
    // lobby->addRenderedEntity(lobby_desk_bounds);
    // RenderRequest lobby_desk_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // int lobby_desk_id = lobby_desk_bounds;
    // lobby->entity_render_requests[lobby_desk_id] = lobby_desk_request;

    Entity lobby_lockers_bounds = registry.create_entity();
    Motion& lobby_lockers_motion = registry.motions.emplace(lobby_lockers_bounds);
    lobby_lockers_motion.position = { centered_x + (scaled_tile_width * 16.8), centered_y + (scaled_tile_height * 2.2) };
    lobby_lockers_motion.scale = { scaled_tile_width * 4.3, scaled_tile_height * 3.7 };
    lobby_lockers_motion.velocity = { 0, 0 };

    Collider& lobby_lockers_collider = registry.colliders.emplace(lobby_lockers_bounds);
    lobby_lockers_collider.type = OBSTACLE;
    lobby_lockers_collider.transparent = true;

    BoundingBox& lobby_lockers_bb = registry.boundingBoxes.emplace(lobby_lockers_bounds);
    lobby_lockers_bb.width = scaled_tile_width * 4.3f;
    lobby_lockers_bb.height = scaled_tile_height * 3.7f;

    lobby->addNonRenderedEntity(lobby_lockers_bounds);

    // Visualize bounding box
    // lobby->addRenderedEntity(lobby_lockers_bounds);
    // RenderRequest lobby_lockers_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // int lobby_lockers_id = lobby_lockers_bounds;
    // lobby->entity_render_requests[lobby_lockers_id] = lobby_lockers_request;

    Entity patrol1 = createPatrol({scaled_tile_width * 5, scaled_tile_height * 5}, 10, scaled_tile_width, scaled_tile_height);
    addPatrolToRoom(renderer, lobby, patrol1);

    Entity patrol2 = createPatrol({ scaled_tile_width * 9, scaled_tile_height * 9 }, 0, scaled_tile_width, scaled_tile_height);
    addPatrolToRoom(renderer, lobby, patrol2);

    Entity lobby_rt_bounds = registry.create_entity();
    Motion& lobby_rt_motion = registry.motions.emplace(lobby_rt_bounds);
    lobby_rt_motion.position = { centered_x + (scaled_tile_width * 19.2), centered_y + (scaled_tile_height * 2.7) };
    lobby_rt_motion.scale = { scaled_tile_width, scaled_tile_height * 6.5 };
    lobby_rt_motion.velocity = { 0,0 };

    Collider& lobby_rt_collider = registry.colliders.emplace(lobby_rt_bounds);
    lobby_rt_collider.type = OBSTACLE;

    BoundingBox& lobby_rt_bb = registry.boundingBoxes.emplace(lobby_rt_bounds);
    lobby_rt_bb.width = scaled_tile_width;
    lobby_rt_bb.height = scaled_tile_height * 6.5f;

    lobby->addNonRenderedEntity(lobby_rt_bounds);

    Entity lobby_rb_bounds = registry.create_entity();
    Motion& lobby_rb_motion = registry.motions.emplace(lobby_rb_bounds);
    lobby_rb_motion.position = { centered_x + (scaled_tile_width * 19.2), centered_y + (scaled_tile_height * 10) };
    lobby_rb_motion.scale = { scaled_tile_width, scaled_tile_height * 6.5 };
    lobby_rb_motion.velocity = { 0,0 };

    Collider& lobby_rb_collider = registry.colliders.emplace(lobby_rb_bounds);
    lobby_rb_collider.type = OBSTACLE;

    BoundingBox& lobby_rb_bb = registry.boundingBoxes.emplace(lobby_rb_bounds);
    lobby_rb_bb.width = scaled_tile_width;
    lobby_rb_bb.height = scaled_tile_height * 6.0f;

    lobby->addNonRenderedEntity(lobby_rb_bounds);

    // Visualize bounding box
    // lobby->addRenderedEntity(lobby_rt_bounds);
    // RenderRequest lobby_rt_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // int lobby_rt_id = lobby_rt_bounds;
    // lobby->entity_render_requests[lobby_rt_id] = lobby_rt_request;

    // BOUNCER NPC ENTITY
    Entity bouncer = registry.create_entity();
    Motion& bouncer_motion = registry.motions.emplace(bouncer);
    bouncer_motion.position = {centered_x + (scaled_tile_width * 18.6), centered_y + (scaled_tile_height *6.2)};
    bouncer_motion.scale = {scaled_tile_width * 0.8, scaled_tile_height*1.2};
    bouncer_motion.velocity = {0, 0};

    Collider& bouncer_collider = registry.colliders.emplace(bouncer);
    bouncer_collider.type = CREATURE;

    BoundingBox& bouncer_bb = registry.boundingBoxes.emplace(bouncer);
    bouncer_bb.width = scaled_tile_width;
    bouncer_bb.height = scaled_tile_height*2;

    Stats& bouncer_stats = registry.stats.emplace(bouncer);
    bouncer_stats.maxHp = 65;
    bouncer_stats.currentHp = 65;
    bouncer_stats.reputation = 75;
    bouncer_stats.cuteness = 65;
    bouncer_stats.ferocity = 75;

    NPC& bouncer_npc = registry.npcs.emplace(bouncer);
    bouncer_npc.encounter_texture_id = (int)TEXTURE_ASSET_ID::TEMP_NPC;
    bouncer_npc.name = "Thug";
    bouncer_npc.isInteractable = true;
    bouncer_npc.dropKey = true;

    bouncer_npc.attackDuration = 10.0f;

    lobby->addNonRenderedEntity(bouncer);
    lobby->addRenderedEntity(bouncer);
    RenderRequest bouncer_request = {
        TEXTURE_ASSET_ID::BOUNCER_LEFT,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };
    int bouncer_id = bouncer;
    lobby->entity_render_requests[bouncer_id] = bouncer_request;

    Entity bouncer_encounter_icon = registry.create_entity();
    Motion& bouncer_icon_motion = registry.motions.emplace(bouncer_encounter_icon);

    bouncer_icon_motion.position = {bouncer_motion.position.x, bouncer_motion.position.y - scaled_tile_height*0.4};
    bouncer_icon_motion.scale = {scaled_tile_width * 0.6, scaled_tile_height * 0.6};
    bouncer_icon_motion.velocity = {0, 0};

    registry.uiElements.emplace(bouncer_encounter_icon);
    
    lobby->addRenderedEntity(bouncer_encounter_icon);
    RenderRequest bouncer_icon_request = {
        TEXTURE_ASSET_ID::ENCOUNTER_ICON,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    bouncer_npc.interactIcon = bouncer_encounter_icon;
    int bouncer_encounter_icon_id = bouncer_encounter_icon;
    lobby->entity_render_requests[bouncer_encounter_icon_id] = bouncer_icon_request;

    Entity lobby_boss_exit = registry.create_entity();
    Motion& lobby_boss_motion = registry.motions.emplace(lobby_boss_exit);
    lobby_boss_motion.position = {centered_x + (scaled_tile_width * 19.2), centered_y + (scaled_tile_height *6)};
    lobby_boss_motion.scale = { scaled_tile_width, scaled_tile_height};
    lobby_boss_motion.velocity = { 0,0 };



    Door& lobby_boss_door = registry.doors.emplace(lobby_boss_exit);
    lobby_boss_door.is_open = false;
    lobby_boss_door.required_keys = 3;
    NPC& door_NPC = registry.npcs.emplace(lobby_boss_exit);
    door_NPC.hasDialogue = true;
    door_NPC.interactDistance = 100;
    // std::string message = std::to_string(lobby_boss_door.required_keys - keyInven.keys.size());
    lobby->addRenderedEntity(lobby_boss_exit);


    Collider& lobby_boss_collider = registry.colliders.emplace(lobby_boss_exit);
    lobby_boss_collider.type = DOOR;


    BoundingBox& lobby_boss_bb = registry.boundingBoxes.emplace(lobby_boss_exit);
    lobby_boss_bb.width = scaled_tile_width;
    lobby_boss_bb.height = scaled_tile_height*2;

    lobby->addNonRenderedEntity(lobby_boss_exit);
    lobby_to_boss_id = lobby_boss_exit;
    level->spawnPositions[lobby_to_boss_id] =  {centered_x + (scaled_tile_width * 18), centered_y + (scaled_tile_height *6.2)};

    // Visualize bounding box
    // lobby->addRenderedEntity(lobby_rb_bounds);
    // RenderRequest lobby_rb_request = {
    //     TEXTURE_ASSET_ID::MID_FLOOR,
    //     EFFECT_ASSET_ID::TEXTURED,
    //     GEOMETRY_BUFFER_ID::SPRITE,
    //     {},
    //     false
    // };
    // int lobby_rb_id = lobby_rb_bounds;
    // lobby->entity_render_requests[lobby_rb_id] = lobby_rb_request;

    Stats sushiStats;
    sushiStats.currentHp = 3;
    Entity sushi = createConsumable({ scaled_tile_width * 6,scaled_tile_height * 8 }, sushiStats, TEXTURE_ASSET_ID::SUSHI);
    lobby->addRenderedEntity(sushi);

    RenderRequest sushi_render_request = {
        TEXTURE_ASSET_ID::SUSHI,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    lobby->entity_render_requests[sushi] = sushi_render_request;


    Stats gemStats;
    gemStats.cuteness = 8;
    Entity gem = createEquippable({ scaled_tile_width * 12,scaled_tile_height * 9 }, gemStats, TEXTURE_ASSET_ID::GEM);
    lobby->addRenderedEntity(gem);

    RenderRequest gem_render_request = {
        TEXTURE_ASSET_ID::GEM,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    lobby->entity_render_requests[gem] = gem_render_request;

    Stats moneyStats;
    moneyStats.reputation = 6;
    Entity money = createEquippable({ scaled_tile_width * 10,scaled_tile_height * 10 }, moneyStats, TEXTURE_ASSET_ID::MONEY);
    lobby->addRenderedEntity(money);

    RenderRequest money_render_request = {
        TEXTURE_ASSET_ID::MONEY,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    lobby->entity_render_requests[money] = money_render_request;
    lobby->room_height = window_height;
    lobby->room_width = window_width;
    level->addRoom(lobby);
}

void createCityMap(Level* level, RenderSystem* renderer) {
    Room* map = new Room("map room");

    map->player_position = {200, 3950};
    level->originalSpawnPositions.push_back(map->player_position);
    map->player_scale = { scaled_tile_width * 1.2, scaled_tile_height * 1.2 };

    Entity background = registry.create_entity();
    RenderRequest background_png = {
        TEXTURE_ASSET_ID::CITY_MAP,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    Motion& background_motion = registry.motions.emplace(background);

    background_motion.scale = {4096, 4096};
    background_motion.position = {2048, 2048};

    // Set room width and height
    map->room_width = background_motion.scale.x;
    map->room_height = background_motion.scale.y;

    map->entity_render_requests[background] = background_png;
    
    map->addRenderedEntity(background);

    map->gridDimensions = {50, 50};

    Entity map_player = registry.players.entities[0];
    int map_player_id = map_player;

    RenderRequest map_player_request;
    map_player_request.used_texture = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    map_player_request.used_effect = EFFECT_ASSET_ID::TEXTURED;
    map_player_request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    map_player_request.hasAnimation = true;
    renderer->setAnimation(map_player_request, AnimationState::MOVING_RIGHT, renderer->catAnimationMap);
    map->entity_render_requests[map_player_id] = map_player_request;

    map->addRenderedEntity(map_player);
    map->addNonRenderedEntity(map_player);

    Entity map_lobby_exit = registry.create_entity();
    Motion& map_lobby_motion = registry.motions.emplace(map_lobby_exit);
    registry.doors.emplace(map_lobby_exit);
    // x = 1560 to 1630
    map_lobby_motion.position = {1595, 510};

    Collider& map_lobby_collider = registry.colliders.emplace(map_lobby_exit);
    map_lobby_collider.type = DOOR;

    BoundingBox& map_lobby_bb = registry.boundingBoxes.emplace(map_lobby_exit);
    map_lobby_bb.width = 70;
    map_lobby_bb.height = 30;

    map->addNonRenderedEntity(map_lobby_exit);
    map_to_lobby_id = map_lobby_exit;
    level->spawnPositions[map_to_lobby_id] =  {1600, 540};

    //level->spawnPositions[map_to_lobby_id];

    Entity map_office_exit = registry.create_entity();
    Motion& map_office_motion = registry.motions.emplace(map_office_exit);
    registry.doors.emplace(map_office_exit);
    map_office_motion.position = {855, 3840};

    Collider& map_office_collider = registry.colliders.emplace(map_office_exit);
    map_office_collider.type = DOOR;

    BoundingBox& map_office_bb = registry.boundingBoxes.emplace(map_office_exit);
    map_office_bb.width = 50;
    map_office_bb.height = 60;

    map->addNonRenderedEntity(map_office_exit);
    map_to_office_id = map_office_exit;
    level->spawnPositions[map_to_office_id] =  {820, 3920};

    Entity map_vet_exit = registry.create_entity();
    Motion& map_vet_motion = registry.motions.emplace(map_vet_exit);
    registry.doors.emplace(map_vet_exit);

    map_vet_motion.position = {820, 2300};

    Collider& map_vet_collider = registry.colliders.emplace(map_vet_exit);
    map_vet_collider.type = DOOR;

    BoundingBox& map_vet_bb = registry.boundingBoxes.emplace(map_vet_exit);
    map_vet_bb.width = 140;
    map_vet_bb.height = 40;

    map->addNonRenderedEntity(map_vet_exit);
    map_to_vet_id = map_vet_exit;
    level->spawnPositions[map_to_vet_id] =  {790, 2400};

    // Bounding Boxes

    // Map Boundaries

    Entity bottom_bound = registry.create_entity();
    Motion& bottom_motion = registry.motions.emplace(bottom_bound);
    bottom_motion.position = {2048, 4096};
    BoundingBox& bottom_bb = registry.boundingBoxes.emplace(bottom_bound);
    bottom_bb.height = 40;
    bottom_bb.width = 4096;
    Collider& bottom_c = registry.colliders.emplace(bottom_bound);
    bottom_c.type = OBSTACLE;
    map->addNonRenderedEntity(bottom_bound);

    Entity top_bound = registry.create_entity();
    Motion& top_motion = registry.motions.emplace(top_bound);
    top_motion.position = {2048, 0};
    BoundingBox& top_bb = registry.boundingBoxes.emplace(top_bound);
    top_bb.height = 40;
    top_bb.width = 4096;
    Collider& top_c = registry.colliders.emplace(top_bound);
    top_c.type = OBSTACLE;
    map->addNonRenderedEntity(top_bound);


    Entity left_bound = registry.create_entity();
    Motion& left_motion = registry.motions.emplace(left_bound);
    left_motion.position = {0, 2048};
    BoundingBox& left_bb = registry.boundingBoxes.emplace(left_bound);
    left_bb.height = 4096;
    left_bb.width = 40;
    Collider& left_c = registry.colliders.emplace(left_bound);
    left_c.type = OBSTACLE;
    map->addNonRenderedEntity(left_bound);


    Entity right_bound = registry.create_entity();
    Motion& right_motion = registry.motions.emplace(right_bound);
    right_motion.position = {4096, 2048};
    BoundingBox& right_bb = registry.boundingBoxes.emplace(right_bound);
    right_bb.height = 4096;
    right_bb.width = 40;
    Collider& right_c = registry.colliders.emplace(right_bound);
    right_c.type = OBSTACLE;
    map->addNonRenderedEntity(right_bound);

    // x = 0 to 740, y = 2700 to 3850 
    // center = 370, 3275 width = 740 height = 1150
    Entity lower_left_1_block = registry.create_entity();
    Motion& lower_left_1_block_motion = registry.motions.emplace(lower_left_1_block);
    lower_left_1_block_motion.position = {370, 3275};

    BoundingBox& lower_left_1_bb = registry.boundingBoxes.emplace(lower_left_1_block);
    lower_left_1_bb.width = 740;
    lower_left_1_bb.height = 1150;

    Collider& lower_left_1_c = registry.colliders.emplace(lower_left_1_block);
    lower_left_1_c.type = OBSTACLE;

    map->addNonRenderedEntity(lower_left_1_block);

    // x = 740 to 990, y = 2700 to 3630 
    // center = 370, 3275 width = 740 height = 1150
    Entity lower_left_2_block = registry.create_entity();
    Motion& lower_left_2_block_motion = registry.motions.emplace(lower_left_2_block);
    lower_left_2_block_motion.position = {865, 3165};

    BoundingBox& lower_left_2_bb = registry.boundingBoxes.emplace(lower_left_2_block);
    lower_left_2_bb.width = 250;
    lower_left_2_bb.height = 930;

    Collider& lower_left_2_c = registry.colliders.emplace(lower_left_2_block);
    lower_left_2_c.type = OBSTACLE;

    map->addNonRenderedEntity(lower_left_2_block);


    // room 1 block
    // x = 740 to 990 y = 3630 to 3850
    Entity office_room_block = registry.create_entity();
    Motion& office_room_block_motion = registry.motions.emplace(office_room_block);
    office_room_block_motion.position = {865, 3705};

    BoundingBox& office_room_bb = registry.boundingBoxes.emplace(office_room_block);
    office_room_bb.width = 250;
    office_room_bb.height = 220;

    Collider& office_room_c = registry.colliders.emplace(office_room_block);
    office_room_c.type = OBSTACLE;
    office_room_c.transparent = true;

    map->addNonRenderedEntity(office_room_block);

    // x = 0 to 660, y = 2040 to 2310 
    // center = 450, 2175 width = 660 height = 270
    Entity middle_left_block = registry.create_entity();
    Motion& middle_left_block_motion = registry.motions.emplace(middle_left_block);
    middle_left_block_motion.position = {330, 2175};

    BoundingBox& middle_left_bb = registry.boundingBoxes.emplace(middle_left_block);
    middle_left_bb.width = 660;
    middle_left_bb.height = 270;

    Collider& middle_left_c = registry.colliders.emplace(middle_left_block);
    middle_left_c.type = OBSTACLE;

    map->addNonRenderedEntity(middle_left_block);

    // x = 660 to 990, y = 2040 to 2310 
    // center = 450, 2175 width = 330 height = 270
    Entity vet_room_block = registry.create_entity();
    Motion& vet_room_block_motion = registry.motions.emplace(vet_room_block);
    vet_room_block_motion.position = {825, 2175};

    BoundingBox& vet_room_bb = registry.boundingBoxes.emplace(vet_room_block);
    vet_room_bb.width = 330;
    vet_room_bb.height = 270;

    Collider& vet_room_c = registry.colliders.emplace(vet_room_block);
    vet_room_c.type = OBSTACLE;
    vet_room_c.transparent = true;

    map->addNonRenderedEntity(vet_room_block);

    // x = 0 to 410, y = 570 to 980 
    // center = 205, 775 width = 410 height = 410
    Entity lake_block = registry.create_entity();
    Motion& lake_block_motion = registry.motions.emplace(lake_block);
    lake_block_motion.position = {205, 775};

    BoundingBox& lake_block_bb = registry.boundingBoxes.emplace(lake_block);
    lake_block_bb.width = 380;
    lake_block_bb.height = 380;

    Collider& lake_block_c = registry.colliders.emplace(lake_block);
    lake_block_c.type = FRICTION;
    lake_block_c.friction = 0.5f;

    map->addNonRenderedEntity(lake_block);


    // grass

    // x = 0 to 410, y = 0 to 570
    // center = 205, 285 width = 410 height = 570
    Entity grass_block_1 = registry.create_entity();
    Motion& grass_block_1_motion = registry.motions.emplace(grass_block_1);
    grass_block_1_motion.position = {205, 285};

    BoundingBox& grass_block_1_bb = registry.boundingBoxes.emplace(grass_block_1);
    grass_block_1_bb.width = 390;
    grass_block_1_bb.height = 550;

    Collider& grass_block_1_c = registry.colliders.emplace(grass_block_1);
    grass_block_1_c.type = FRICTION;
    grass_block_1_c.friction = 0.2f;

    map->addNonRenderedEntity(grass_block_1);

    // x = 410 to 980, y = 0 to 2040
    // center = 695, 1020 width = 570 height = 2040
    Entity grass_block_2 = registry.create_entity();
    Motion& grass_block_2_motion = registry.motions.emplace(grass_block_2);
    grass_block_2_motion.position = {695, 1020};

    BoundingBox& grass_block_2_bb = registry.boundingBoxes.emplace(grass_block_2);
    grass_block_2_bb.width = 550;
    grass_block_2_bb.height = 2020;

    Collider& grass_block_2_c = registry.colliders.emplace(grass_block_2);
    grass_block_2_c.type = FRICTION;
    grass_block_2_c.friction = 0.3f;

    map->addNonRenderedEntity(grass_block_2);

    // x = 0 to 410, y = 980 to 2040
    // center = 205, 1510 width = 410 height = 1060
    Entity grass_block_3 = registry.create_entity();
    Motion& grass_block_3_motion = registry.motions.emplace(grass_block_3);
    grass_block_3_motion.position = {205, 1510};

    BoundingBox& grass_block_3_bb = registry.boundingBoxes.emplace(grass_block_3);
    grass_block_3_bb.width = 390;
    grass_block_3_bb.height = 1040;

    Collider& grass_block_3_c = registry.colliders.emplace(grass_block_3);
    grass_block_3_c.type = FRICTION;
    grass_block_3_c.friction = 0.3f;

    map->addNonRenderedEntity(grass_block_3);

    // big grass
    // x = 1620 to 4096, y = 3200 to 3680
    // center = 2858, 3440 width = 2476 height = 480
    Entity big_grass = registry.create_entity();
    Motion& big_grass_motion = registry.motions.emplace(big_grass);
    big_grass_motion.position = {2858, 3440};

    BoundingBox& big_grass_bb = registry.boundingBoxes.emplace(big_grass);
    big_grass_bb.width = 2476;
    big_grass_bb.height = 480;

    Collider& big_grass_c = registry.colliders.emplace(big_grass);
    big_grass_c.type = FRICTION;
    big_grass_c.friction = 0.3f;

    map->addNonRenderedEntity(big_grass);

    // unfortunately the dog doesnt have up and down animations, so we only use left and flip it for right.
    Stats stats;
    Entity dog = createNPC({300, 200}, -tile_width * 4, tile_height * 4, stats, "dog", false, 0.0f, TEXTURE_ASSET_ID::TEXTURE_COUNT);
    addNPCToRoom(renderer, map, dog, TEXTURE_ASSET_ID::WHITE_DOG, AnimationState::ON_TWO_FEET, renderer->dogAnimationMap);

    Entity dog2 = createNPC({150, 250}, -tile_width * 4, tile_height * 4, stats, "dog", false, 0.0f, TEXTURE_ASSET_ID::TEXTURE_COUNT);
    addNPCToRoom(renderer, map, dog2, TEXTURE_ASSET_ID::BROWN_DOG, AnimationState::SITTING, renderer->dogAnimationMap);

    Entity dog3 = createNPC({500, 300}, -tile_width * 4, tile_height * 4, stats, "dog", false, 0.0f, TEXTURE_ASSET_ID::TEXTURE_COUNT);
    addNPCToRoom(renderer, map, dog3, TEXTURE_ASSET_ID::WHITE_BROWN_DOG, AnimationState::LAYING_DOWN, renderer->dogAnimationMap);

    Entity dog4 = createNPC({500, 400}, -tile_width * 4, tile_height * 4, stats, "dog", false, 0.0f, TEXTURE_ASSET_ID::TEXTURE_COUNT);
    addNPCToRoom(renderer, map, dog4, TEXTURE_ASSET_ID::BROWN_BLACK_DOG, AnimationState::ON_TWO_FEET, renderer->dogAnimationMap);

    // x = 1380 to 1620, y = 3200 to 4096 
    // center = 1500, 3648 width = 240 height = 896
    Entity middle_apt_block = registry.create_entity();
    Motion& middle_apt_block_motion = registry.motions.emplace(middle_apt_block);
    middle_apt_block_motion.position = {1510, 3648};

    BoundingBox& middle_apt_block_bb = registry.boundingBoxes.emplace(middle_apt_block);
    middle_apt_block_bb.width = 270;
    middle_apt_block_bb.height = 916;

    Collider& middle_apt_block_c = registry.colliders.emplace(middle_apt_block);
    middle_apt_block_c.type = OBSTACLE;

    map->addNonRenderedEntity(middle_apt_block);

    // x = 1380 to 2060, y = 2700 to 3200 
    // center = 1720, 2950 width = 680 height = 500
    Entity middle_sqr_block = registry.create_entity();
    Motion& middle_sqr_block_motion = registry.motions.emplace(middle_sqr_block);
    middle_sqr_block_motion.position = {1720, 2950};

    BoundingBox& middle_sqr_block_bb = registry.boundingBoxes.emplace(middle_sqr_block);
    middle_sqr_block_bb.width = 690;
    middle_sqr_block_bb.height = 520;

    Collider& middle_sqr_block_c = registry.colliders.emplace(middle_sqr_block);
    middle_sqr_block_c.type = OBSTACLE;

    map->addNonRenderedEntity(middle_sqr_block);

    // x = 1620 to 4096, y = 3680 to 4096 
    // center = 2858, 3888 width = 2476 height = 416
    Entity lower_right_block = registry.create_entity();
    Motion& lower_right_block_motion = registry.motions.emplace(lower_right_block);
    lower_right_block_motion.position = {2858, 3888};

    BoundingBox& lower_right_block_bb = registry.boundingBoxes.emplace(lower_right_block);
    lower_right_block_bb.width = 2496;
    lower_right_block_bb.height = 436;

    Collider& lower_right_block_c = registry.colliders.emplace(lower_right_block);
    lower_right_block_c.type = OBSTACLE;

    map->addNonRenderedEntity(lower_right_block);

    // x = 2200 to 4096, y = 2700 to 3200 
    // center = 3148, 2950 width = 680 height = 500
    Entity middle_right_block = registry.create_entity();
    Motion& middle_right_block_motion = registry.motions.emplace(middle_right_block);
    middle_right_block_motion.position = {3148, 2950};

    BoundingBox& middle_right_block_bb = registry.boundingBoxes.emplace(middle_right_block);
    middle_right_block_bb.width = 1916;
    middle_right_block_bb.height = 520;

    Collider& middle_right_block_c = registry.colliders.emplace(middle_right_block);
    middle_right_block_c.type = OBSTACLE;

    map->addNonRenderedEntity(middle_right_block);

    // x = 1380 to 4096, y = 800 to 2310 
    // center = 2738, 1555 width = 2716 height = 1510
    Entity big_apt_block = registry.create_entity();
    Motion& big_apt_block_motion = registry.motions.emplace(big_apt_block);
    big_apt_block_motion.position = {2738, 1555};

    BoundingBox& big_apt_block_bb = registry.boundingBoxes.emplace(big_apt_block);
    big_apt_block_bb.width = 2726;
    big_apt_block_bb.height = 1530;

    Collider& big_apt_block_c = registry.colliders.emplace(big_apt_block);
    big_apt_block_c.type = OBSTACLE;

    map->addNonRenderedEntity(big_apt_block);

    // x = 1380 to 4096, y = 0 to 260 
    // center = 2738, 260 width = 2716 height = 260
    Entity top_apt_1_block = registry.create_entity();
    Motion& top_apt_1_block_motion = registry.motions.emplace(top_apt_1_block);
    top_apt_1_block_motion.position = {2738, 130};

    BoundingBox& top_apt_1_block_bb = registry.boundingBoxes.emplace(top_apt_1_block);
    top_apt_1_block_bb.width = 2736;
    top_apt_1_block_bb.height = 260;

    Collider& top_apt_1_block_c = registry.colliders.emplace(top_apt_1_block);
    top_apt_1_block_c.type = OBSTACLE;

    map->addNonRenderedEntity(top_apt_1_block);

    // x = 1800 to 4096, y = 260 to 520 
    // center = 2948, 260 width = 2296 height = 260
    Entity top_apt_2_block = registry.create_entity();
    Motion& top_apt_2_block_motion = registry.motions.emplace(top_apt_2_block);
    top_apt_2_block_motion.position = {2948, 390};

    BoundingBox& top_apt_2_block_bb = registry.boundingBoxes.emplace(top_apt_2_block);
    top_apt_2_block_bb.width = 2296;
    top_apt_2_block_bb.height = 260;

    Collider& top_apt_2_block_c = registry.colliders.emplace(top_apt_2_block);
    top_apt_2_block_c.type = OBSTACLE;

    map->addNonRenderedEntity(top_apt_2_block);

    // x = 1380 to 1800, y = 260 to 520 
    // center = 2738, 260 width = 2716 height = 520
    Entity lobby_room_block = registry.create_entity();
    Motion& lobby_room_block_motion = registry.motions.emplace(lobby_room_block);
    lobby_room_block_motion.position = {1590, 390};

    BoundingBox& lobby_room_block_bb = registry.boundingBoxes.emplace(lobby_room_block);
    lobby_room_block_bb.width = 420;
    lobby_room_block_bb.height = 260;

    Collider& lobby_room_block_c = registry.colliders.emplace(lobby_room_block);
    lobby_room_block_c.type = OBSTACLE;
    lobby_room_block_c.transparent = true;

    map->addNonRenderedEntity(lobby_room_block);

    // x = 1140 to 1380, y = 0 to 260 
    // center = 1260, 130 width = 240 height = 260
    Entity top_car_block = registry.create_entity();
    Motion& top_car_block_motion = registry.motions.emplace(top_car_block);
    top_car_block_motion.position = {1260, 130};

    BoundingBox& top_car_block_bb = registry.boundingBoxes.emplace(top_car_block);
    top_car_block_bb.width = 250;
    top_car_block_bb.height = 270;

    Collider& top_car_block_c = registry.colliders.emplace(top_car_block);
    top_car_block_c.type = OBSTACLE;
    top_car_block_c.transparent = true;

    map->addNonRenderedEntity(top_car_block);

    // x = 3420, y = 520 to 800 
    // center = 3420, 660 width = 20 height = 280 -- width is irrelevant here
    Entity truck_block = registry.create_entity();
    Motion& truck_block_motion = registry.motions.emplace(truck_block);
    truck_block_motion.position = {3420, 660};

    BoundingBox& truck_block_bb = registry.boundingBoxes.emplace(truck_block);
    truck_block_bb.width = 20;
    truck_block_bb.height = 290;

    Collider& truck_block_c = registry.colliders.emplace(truck_block);
    truck_block_c.type = OBSTACLE;
    truck_block_c.transparent = true;

    map->addNonRenderedEntity(truck_block);

    // x = 1300 to 1380, y = 4020 to 4096 
    // center = 1340, 4058  width = 80 height = 76
    Entity fire_hydrant_block = registry.create_entity();
    Motion& fire_hydrant_block_motion = registry.motions.emplace(fire_hydrant_block);
    fire_hydrant_block_motion.position = {1340, 4058};

    BoundingBox& fire_hydrant_block_bb = registry.boundingBoxes.emplace(fire_hydrant_block);
    fire_hydrant_block_bb.width = 80;
    fire_hydrant_block_bb.height = 100;

    Collider& fire_hydrant_block_c = registry.colliders.emplace(fire_hydrant_block);
    fire_hydrant_block_c.type = OBSTACLE;
    fire_hydrant_block_c.transparent = true;

    map->addNonRenderedEntity(fire_hydrant_block);

    // x = 1160 to 1300, y = 3720 to 4020 
    // center = 1230, 3870  width = 140 height = 300
    Entity police_car_block = registry.create_entity();
    Motion& police_car_block_motion = registry.motions.emplace(police_car_block);
    police_car_block_motion.position = {1230, 3890};

    BoundingBox& police_car_block_bb = registry.boundingBoxes.emplace(police_car_block);
    police_car_block_bb.width = 150;
    police_car_block_bb.height = 270;

    Collider& police_car_block_c = registry.colliders.emplace(police_car_block);
    police_car_block_c.type = OBSTACLE;
    police_car_block_c.transparent = true;

    map->addNonRenderedEntity(police_car_block);

    // Patrols

    Entity patrol1 = createPatrol({2500, 2500}, 0, scaled_tile_width, scaled_tile_height);
    addPatrolToRoom(renderer, map, patrol1);

    Entity patrol2 = createPatrol({1500, 500}, 0, scaled_tile_width, scaled_tile_height);
    addPatrolToRoom(renderer, map, patrol2);

    Entity patrol3 = createPatrol({2300, 640}, 0, scaled_tile_width, scaled_tile_height);
    addPatrolToRoom(renderer, map, patrol3);

    // NPC and items
    // x = 3720 y = 3340
    Entity strong_npc = registry.create_entity();

    Motion& strong_npc_motion = registry.motions.emplace(strong_npc);
    // strong_npc_motion.position = {centered_x + (scaled_tile_width * 15.6), centered_y + (scaled_tile_height *6.2)};
    strong_npc_motion.position = {1820, 3340};
    strong_npc_motion.scale = {scaled_tile_width * 2.4, scaled_tile_height*2};
    strong_npc_motion.velocity = {0, 0};

    Collider& strong_npc_collider = registry.colliders.emplace(strong_npc);
    strong_npc_collider.type = CREATURE;

    BoundingBox& strong_npc_bb = registry.boundingBoxes.emplace(strong_npc);
    strong_npc_bb.width = scaled_tile_width;
    strong_npc_bb.height = scaled_tile_height*1.6;

    Stats& strong_npc_stats = registry.stats.emplace(strong_npc);
    strong_npc_stats.maxHp = 90;
    strong_npc_stats.currentHp = 90;
    strong_npc_stats.reputation = 85;
    strong_npc_stats.cuteness = 85;
    strong_npc_stats.ferocity = 100;

    Animation strong_npc_animation;
    strong_npc_animation.columns = 11;
    strong_npc_animation.currentFrame = 0;
    strong_npc_animation.frameCount = 11;
    strong_npc_animation.rows = 1;
    strong_npc_animation.startCol = 0;
    strong_npc_animation.startRow = 0;
    strong_npc_animation.elapsedTime = 0;
    strong_npc_animation.frameTime = 100;

    NPC& strong_npc_comp = registry.npcs.emplace(strong_npc);
    strong_npc_comp.encounter_texture_id = (int)TEXTURE_ASSET_ID::MAFIA_ENCOUNTER;
    strong_npc_comp.name = "Mafia";
    strong_npc_comp.isInteractable = true;

    strong_npc_comp.attackDuration = 10.0f;

    map->addNonRenderedEntity(strong_npc);
    map->addRenderedEntity(strong_npc);
    RenderRequest strong_npc_request = {
        TEXTURE_ASSET_ID::STRONG_NPC_IDLE,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        strong_npc_animation,
        true
    };
    int strong_npc_id = strong_npc;
    map->entity_render_requests[strong_npc_id] = strong_npc_request;

    Stats food1_stats;
    food1_stats.currentHp = 20;
    Entity food1 = createConsumable({280, 3900}, food1_stats, TEXTURE_ASSET_ID::STEAK);
    map->addRenderedEntity(food1);

    RenderRequest food1_render_request = {
        TEXTURE_ASSET_ID::STEAK,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    map->entity_render_requests[food1] = food1_render_request;

    level->addRoom(map);
}

void createOfficeRoom(Level* level, RenderSystem* renderer) {
    
    Room* office = new Room("office room");

    office_to_map_id = registry.create_entity();
    // office->player_position = {window_width/2 - 350, window_height/2 - 240};
    office->player_scale = { scaled_tile_width * 1.2, scaled_tile_height * 1.2 };

    Entity background = registry.create_entity();
    RenderRequest background_png = {
        TEXTURE_ASSET_ID::OFFICE_ROOM,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    Motion& background_motion = registry.motions.emplace(background);

    background_motion.scale = {1024, 1024};
    float y_scale = window_height/2;
    if (y_scale/2 < 512)
    {
        y_scale += (512 - window_height/2);
    }

    background_motion.position = {window_width/2, y_scale};

    office->entity_render_requests[background] = background_png;
    
    office->addRenderedEntity(background);

    office->gridDimensions = {15, 15};

    Entity office_player = registry.players.entities[0];
    int office_player_id = office_player;
    RenderRequest office_player_request;
    office_player_request.used_texture = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    office_player_request.used_effect = EFFECT_ASSET_ID::TEXTURED;
    office_player_request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    office_player_request.hasAnimation = true;
    renderer->setAnimation(office_player_request, AnimationState::MOVING_RIGHT, renderer->catAnimationMap);

    office->entity_render_requests[office_player_id] = office_player_request;


    office->addRenderedEntity(office_player);
    office->addNonRenderedEntity(office_player);


    // Bounding Boxes

    // We need to use relative position using window_width/2 and window_height/2 as our center

    int center_x = window_width / 2;
    int center_y = y_scale;


    // the 0, 0 relative to window is center_x - 512, center_y - 512
    int x_0 = center_x - 512;
    int y_0 = center_y - 512;


    // Set room width and height
    office->room_width = background_motion.scale.x + x_0;
    office->room_height = background_motion.scale.y + y_0;

    Entity office_map_exit = registry.create_entity();
    Motion& office_map_motion = registry.motions.emplace(office_map_exit);
    registry.doors.emplace(office_map_exit);
    office_map_motion.position = {x_0 + 35, y_0 + 160};

    office->player_position = {office_map_motion.position.x + office->player_scale.x, office_map_motion.position.y};

    Collider& office_map_collider = registry.colliders.emplace(office_map_exit);
    office_map_collider.type = DOOR;

    BoundingBox& office_map_bb = registry.boundingBoxes.emplace(office_map_exit);
    office_map_bb.width = 70;
    office_map_bb.height = 100;

    office->addNonRenderedEntity(office_map_exit);
    office_to_map_id = office_map_exit;
    level->spawnPositions[office_to_map_id] =  {window_width/2 - 350, window_height/2 - 240};


    Entity bottom_bound = registry.create_entity();
    Motion& bottom_motion = registry.motions.emplace(bottom_bound);
    bottom_motion.position = {center_x, center_y + 512};
    BoundingBox& bottom_bb = registry.boundingBoxes.emplace(bottom_bound);
    bottom_bb.height = 80;
    bottom_bb.width = 1024;
    Collider& bottom_c = registry.colliders.emplace(bottom_bound);
    bottom_c.type = OBSTACLE;
    office->addNonRenderedEntity(bottom_bound);

    Entity top_bound = registry.create_entity();
    Motion& top_motion = registry.motions.emplace(top_bound);
    top_motion.position = {center_x, center_y - 512};
    BoundingBox& top_bb = registry.boundingBoxes.emplace(top_bound);
    top_bb.height = 40;
    top_bb.width = 1024;
    Collider& top_c = registry.colliders.emplace(top_bound);
    top_c.type = OBSTACLE;
    office->addNonRenderedEntity(top_bound);


    Entity left_bound = registry.create_entity();
    Motion& left_motion = registry.motions.emplace(left_bound);
    left_motion.position = {center_x - 512, center_y};
    BoundingBox& left_bb = registry.boundingBoxes.emplace(left_bound);
    left_bb.height = 1024;
    left_bb.width = 40;
    Collider& left_c = registry.colliders.emplace(left_bound);
    left_c.type = OBSTACLE;
    office->addNonRenderedEntity(left_bound);


    Entity right_bound = registry.create_entity();
    Motion& right_motion = registry.motions.emplace(right_bound);
    right_motion.position = {center_x + 512, center_y};
    BoundingBox& right_bb = registry.boundingBoxes.emplace(right_bound);
    right_bb.height = 1024;
    right_bb.width = 40;
    Collider& right_c = registry.colliders.emplace(right_bound);
    right_c.type = OBSTACLE;
    office->addNonRenderedEntity(right_bound);

    // x = 135 to 545 , y = 0 to 130
    // center = 340, 65 width = 410 height = 130
    Entity bookshelves_bound = registry.create_entity();
    Motion& bookshelves_bound_motion = registry.motions.emplace(bookshelves_bound);
    bookshelves_bound_motion.position = {x_0 + 340, y_0 + 65};
    BoundingBox& bookshelves_bound_bb = registry.boundingBoxes.emplace(bookshelves_bound);
    bookshelves_bound_bb.width = 430;
    bookshelves_bound_bb.height = 130;
    Collider& bookshelves_bound_c = registry.colliders.emplace(bookshelves_bound);
    bookshelves_bound_c.type = OBSTACLE;
    bookshelves_bound_c.transparent = true;
    office->addNonRenderedEntity(bookshelves_bound);


    // x = 690 to 870 , y = 120 to 340
    // center = 780, 230 width = 180 height = 220
    Entity desk_bound = registry.create_entity();
    Motion& desk_bound_motion = registry.motions.emplace(desk_bound);
    desk_bound_motion.position = {x_0 + 785, y_0 + 220};
    BoundingBox& desk_bound_bb = registry.boundingBoxes.emplace(desk_bound);
    desk_bound_bb.width = 200;
    desk_bound_bb.height = 225;
    Collider& desk_bound_c = registry.colliders.emplace(desk_bound);
    desk_bound_c.type = OBSTACLE;
    desk_bound_c.transparent = true;
    office->addNonRenderedEntity(desk_bound);


    // x = 870 to 950 , y = 160 to 270
    // center = 910, 215 width = 80 height = 110
    Entity chair_bound = registry.create_entity();
    Motion& chair_bound_motion = registry.motions.emplace(chair_bound);
    chair_bound_motion.position = {x_0 + 910, y_0 + 215};
    BoundingBox& chair_bound_bb = registry.boundingBoxes.emplace(chair_bound);
    chair_bound_bb.width = 100;
    chair_bound_bb.height = 120;
    Collider& chair_bound_c = registry.colliders.emplace(chair_bound);
    chair_bound_c.type = OBSTACLE;
    chair_bound_c.transparent = true;
    office->addNonRenderedEntity(chair_bound);

    // x = 750 to 820 , y = 70 to 270
    // center = 785, 220 width = 70 height = 100
    Entity head_bound = registry.create_entity();
    Motion& head_bound_motion = registry.motions.emplace(head_bound);
    head_bound_motion.position = {x_0 + 785, y_0 + 120};
    BoundingBox& head_bound_bb = registry.boundingBoxes.emplace(head_bound);
    head_bound_bb.width = 95;
    head_bound_bb.height = 130;
    Collider& head_bound_c = registry.colliders.emplace(head_bound);
    head_bound_c.type = OBSTACLE;
    head_bound_c.transparent = true;
    office->addNonRenderedEntity(head_bound);

    // x = 290 to 460 , y = 650 to 750
    // center = 375, 700 width = 170 height = 100
    Entity table_bound = registry.create_entity();
    Motion& table_bound_motion = registry.motions.emplace(table_bound);
    table_bound_motion.position = {x_0 + 375, y_0 + 685};
    BoundingBox& table_bound_bb = registry.boundingBoxes.emplace(table_bound);
    table_bound_bb.width = 190;
    table_bound_bb.height = 110;
    Collider& table_bound_c = registry.colliders.emplace(table_bound);
    table_bound_c.type = OBSTACLE;
    table_bound_c.transparent = true;
    office->addNonRenderedEntity(table_bound);

    // x = 340 to 400 , y = 620 to 700
    // center = 370, 660 width = 60 height = 80
    Entity small_chair_bound = registry.create_entity();
    Motion& small_chair_bound_motion = registry.motions.emplace(small_chair_bound);
    small_chair_bound_motion.position = {x_0 + 370, y_0 + 655};
    BoundingBox& small_chair_bound_bb = registry.boundingBoxes.emplace(small_chair_bound);
    small_chair_bound_bb.width = 70;
    small_chair_bound_bb.height = 100;
    Collider& small_chair_bound_c = registry.colliders.emplace(small_chair_bound);
    small_chair_bound_c.type = OBSTACLE;
    small_chair_bound_c.transparent = true;
    office->addNonRenderedEntity(small_chair_bound);

    // x = 610 to 730 , y = 650 to 750
    // center = 670, 700 width = 120 height = 100
    Entity piano_bound = registry.create_entity();
    Motion& piano_bound_motion = registry.motions.emplace(piano_bound);
    piano_bound_motion.position = {x_0 + 670, y_0 + 690};
    BoundingBox& piano_bound_bb = registry.boundingBoxes.emplace(piano_bound);
    piano_bound_bb.width = 130;
    piano_bound_bb.height = 120;
    Collider& piano_bound_c = registry.colliders.emplace(piano_bound);
    piano_bound_c.type = OBSTACLE;
    piano_bound_c.transparent = true;
    office->addNonRenderedEntity(piano_bound);



    // BOUNCER NPC ENTITY
    Entity bouncer = registry.create_entity();
    Motion& bouncer_motion = registry.motions.emplace(bouncer);
    // bouncer_motion.position = {centered_x + (scaled_tile_width * 15.6), centered_y + (scaled_tile_height *6.2)};
    bouncer_motion.position = {center_x, center_y};
    bouncer_motion.scale = {scaled_tile_width * 0.8, scaled_tile_height*1.2};
    bouncer_motion.velocity = {0, 0};

    Collider& bouncer_collider = registry.colliders.emplace(bouncer);
    bouncer_collider.type = CREATURE;

    BoundingBox& bouncer_bb = registry.boundingBoxes.emplace(bouncer);
    bouncer_bb.width = scaled_tile_width;
    bouncer_bb.height = scaled_tile_height;

    Stats& bouncer_stats = registry.stats.emplace(bouncer);
    bouncer_stats.maxHp = 25;
    bouncer_stats.currentHp = 25;
    bouncer_stats.reputation = 40;
    bouncer_stats.cuteness = 25;
    bouncer_stats.ferocity = 30;

    NPC& bouncer_npc = registry.npcs.emplace(bouncer);
    bouncer_npc.encounter_texture_id = (int)TEXTURE_ASSET_ID::TEMP_NPC;
    bouncer_npc.name = "Thug 1";
    bouncer_npc.isInteractable = true;
    bouncer_npc.dropKey = true;

    bouncer_npc.attackDuration = 10.0f;

    bouncer_npc.dialogue.push("Thug: What a cute cat... ");
    bouncer_npc.dialogue.push("Thug: Ahem, I mean I'M A BIG MEAN SCARY GUY");

    office->addNonRenderedEntity(bouncer);
    office->addRenderedEntity(bouncer);
    RenderRequest bouncer_request = {
        TEXTURE_ASSET_ID::BOUNCER_LEFT,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };
    int bouncer_id = bouncer;
    office->entity_render_requests[bouncer_id] = bouncer_request;

    Entity bouncer_encounter_icon = registry.create_entity();
    Motion& bouncer_icon_motion = registry.motions.emplace(bouncer_encounter_icon);

    bouncer_icon_motion.position = {bouncer_motion.position.x, bouncer_motion.position.y - scaled_tile_height*0.4};
    bouncer_icon_motion.scale = {scaled_tile_width * 0.6, scaled_tile_height * 0.6};
    bouncer_icon_motion.velocity = {0, 0};

    registry.uiElements.emplace(bouncer_encounter_icon);
    
    office->addRenderedEntity(bouncer_encounter_icon);
    RenderRequest bouncer_icon_request = {
        TEXTURE_ASSET_ID::ENCOUNTER_ICON,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    bouncer_npc.interactIcon = bouncer_encounter_icon;
    int bouncer_encounter_icon_id = bouncer_encounter_icon;
    office->entity_render_requests[bouncer_encounter_icon_id] = bouncer_icon_request;
    
    Stats moneyStats;
    moneyStats.reputation = 10;
    Entity money = createEquippable({ scaled_tile_width * 9,scaled_tile_height * 5 }, moneyStats, TEXTURE_ASSET_ID::MONEY);
    office->addRenderedEntity(money);

    RenderRequest money_render_request = {
        TEXTURE_ASSET_ID::MONEY,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    office->entity_render_requests[money] = money_render_request;

    Stats gold_necklace_stats;
    gold_necklace_stats.cuteness = 8;
    Entity gold_necklace = createEquippable({ scaled_tile_width * 7,scaled_tile_height * 8 }, gold_necklace_stats, TEXTURE_ASSET_ID::GOLD_NECKLACE);
    office->addRenderedEntity(gold_necklace);

    RenderRequest gold_necklace_render_request = {
        TEXTURE_ASSET_ID::GOLD_NECKLACE,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    office->entity_render_requests[gold_necklace] = gold_necklace_render_request;
    level->addRoom(office);


}

void createVetRoom(Level* level,RenderSystem* renderer){
    Room* vet = new Room("vet room");

    vet_to_map_id = registry.create_entity();

    // vet->player_position = {window_width/2 - 350, window_height/2 - 240};
    vet->player_scale = { scaled_tile_width * 1.2, scaled_tile_height * 1.2 };

    Entity background = registry.create_entity();
    RenderRequest background_png = {
        TEXTURE_ASSET_ID::VET_ROOM,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    float y_scale = window_height/2;
    if (y_scale/2 < 512)
    {
        y_scale += (512 - window_height/2);
    }

    Motion& background_motion = registry.motions.emplace(background);

    background_motion.scale = {1024, 1024};
    background_motion.position = {window_width/2, y_scale};

    vet->entity_render_requests[background] = background_png;
    
    vet->addRenderedEntity(background);

    vet->gridDimensions = {15, 15};

    Entity vet_player = registry.players.entities[0];
    int vet_player_id = vet_player;

    RenderRequest vet_player_request;
    vet_player_request.used_texture = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    vet_player_request.used_effect = EFFECT_ASSET_ID::TEXTURED;
    vet_player_request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    vet_player_request.hasAnimation = true;
    renderer->setAnimation(vet_player_request, AnimationState::MOVING_RIGHT, renderer->catAnimationMap);

    vet->entity_render_requests[vet_player_id] = vet_player_request;


    vet->addRenderedEntity(vet_player);
    vet->addNonRenderedEntity(vet_player);


    // Bounding Boxes

    // We need to use relative position using window_width/2 and window_height/2 as our center

    int center_x = window_width / 2;
    int center_y = y_scale;

    
    // the 0, 0 relative to window is center_x - 512, center_y - 512
    int x_0 = center_x - 512;
    int y_0 = center_y - 512;


    // Set room width and height
    vet->room_width = background_motion.scale.x + x_0;
    vet->room_height = background_motion.scale.y + y_0;

    Entity vet_map_exit = registry.create_entity();
    Motion& vet_map_motion = registry.motions.emplace(vet_map_exit);
    registry.doors.emplace(vet_map_exit);
    vet_map_motion.position = {x_0 + 35, y_0 + 160};

    vet->player_position = {vet_map_motion.position.x + vet->player_scale.x, vet_map_motion.position.y};

    Collider& vet_map_collider = registry.colliders.emplace(vet_map_exit);
    vet_map_collider.type = DOOR;

    BoundingBox& vet_map_bb = registry.boundingBoxes.emplace(vet_map_exit);
    vet_map_bb.width = 70;
    vet_map_bb.height = 100;

    vet->addNonRenderedEntity(vet_map_exit);
    vet_to_map_id = vet_map_exit;
    level->spawnPositions[vet_to_map_id] =  {window_width/2 - 350, window_height/2 - 240};


    Entity bottom_bound = registry.create_entity();
    Motion& bottom_motion = registry.motions.emplace(bottom_bound);
    bottom_motion.position = {center_x, center_y + 512};
    BoundingBox& bottom_bb = registry.boundingBoxes.emplace(bottom_bound);
    bottom_bb.height = 80;
    bottom_bb.width = 1024;
    Collider& bottom_c = registry.colliders.emplace(bottom_bound);
    bottom_c.type = OBSTACLE;
    vet->addNonRenderedEntity(bottom_bound);

    Entity top_bound = registry.create_entity();
    Motion& top_motion = registry.motions.emplace(top_bound);
    top_motion.position = {center_x, center_y - 512};
    BoundingBox& top_bb = registry.boundingBoxes.emplace(top_bound);
    top_bb.height = 40;
    top_bb.width = 1024;
    Collider& top_c = registry.colliders.emplace(top_bound);
    top_c.type = OBSTACLE;
    vet->addNonRenderedEntity(top_bound);


    Entity left_bound = registry.create_entity();
    Motion& left_motion = registry.motions.emplace(left_bound);
    left_motion.position = {center_x - 512, center_y};
    BoundingBox& left_bb = registry.boundingBoxes.emplace(left_bound);
    left_bb.height = 1024;
    left_bb.width = 40;
    Collider& left_c = registry.colliders.emplace(left_bound);
    left_c.type = OBSTACLE;
    vet->addNonRenderedEntity(left_bound);


    Entity right_bound = registry.create_entity();
    Motion& right_motion = registry.motions.emplace(right_bound);
    right_motion.position = {center_x + 512, center_y};
    BoundingBox& right_bb = registry.boundingBoxes.emplace(right_bound);
    right_bb.height = 1024;
    right_bb.width = 40;
    Collider& right_c = registry.colliders.emplace(right_bound);
    right_c.type = OBSTACLE;
    vet->addNonRenderedEntity(right_bound);

    // x = 130 to 480 , y = 0 to 120
    // center = 305, 60 width = 410 height = 130
    Entity chairs_bound = registry.create_entity();
    Motion& chairs_bound_motion = registry.motions.emplace(chairs_bound);
    chairs_bound_motion.position = {x_0 + 305, y_0 + 60};
    BoundingBox& chairs_bound_bb = registry.boundingBoxes.emplace(chairs_bound);
    chairs_bound_bb.width = 350;
    chairs_bound_bb.height = 130;
    Collider& chairs_bound_c = registry.colliders.emplace(chairs_bound);
    chairs_bound_c.type = OBSTACLE;
    chairs_bound_c.transparent = true;
    vet->addNonRenderedEntity(chairs_bound);

    // x = 550 to 1030 , y = 0 to 410
    // center = 790, 205 width = 480 height = 410
    Entity top_left_bound = registry.create_entity();
    Motion& top_left_bound_motion = registry.motions.emplace(top_left_bound);
    top_left_bound_motion.position = {x_0 + 790, y_0 + 205};
    BoundingBox& top_left_bound_bb = registry.boundingBoxes.emplace(top_left_bound);
    top_left_bound_bb.width = 480;
    top_left_bound_bb.height = 390;
    Collider& top_left_bound_c = registry.colliders.emplace(top_left_bound);
    top_left_bound_c.type = OBSTACLE;
    top_left_bound_c.transparent = true;
    vet->addNonRenderedEntity(top_left_bound);

    // x = 670 to 890 , y = 510 to 610
    // center = 780, 560 width = 220 height = 100
    Entity bed1_bound = registry.create_entity();
    Motion& bed1_bound_motion = registry.motions.emplace(bed1_bound);
    bed1_bound_motion.position = {x_0 + 780, y_0 + 550};
    BoundingBox& bed1_bound_bb = registry.boundingBoxes.emplace(bed1_bound);
    bed1_bound_bb.width = 220;
    bed1_bound_bb.height = 100;
    Collider& bed1_bound_c = registry.colliders.emplace(bed1_bound);
    bed1_bound_c.type = OBSTACLE;
    bed1_bound_c.transparent = true;
    vet->addNonRenderedEntity(bed1_bound);

    // x = 670 to 890 , y = 640 to 740
    // center = 780, 690 width = 220 height = 100
    Entity bed2_bound = registry.create_entity();
    Motion& bed2_bound_motion = registry.motions.emplace(bed2_bound);
    bed2_bound_motion.position = {x_0 + 780, y_0 + 680};
    BoundingBox& bed2_bound_bb = registry.boundingBoxes.emplace(bed2_bound);
    bed2_bound_bb.width = 220;
    bed2_bound_bb.height = 100;
    Collider& bed2_bound_c = registry.colliders.emplace(bed2_bound);
    bed2_bound_c.type = OBSTACLE;
    bed2_bound_c.transparent = true;
    vet->addNonRenderedEntity(bed2_bound);

    // x = 670 to 890 , y = 790 to 880
    // center = 780, 840 width = 220 height = 100
    Entity bed3_bound = registry.create_entity();
    Motion& bed3_bound_motion = registry.motions.emplace(bed3_bound);
    bed3_bound_motion.position = {x_0 + 780, y_0 + 820};
    BoundingBox& bed3_bound_bb = registry.boundingBoxes.emplace(bed3_bound);
    bed3_bound_bb.width = 220;
    bed3_bound_bb.height = 100;
    Collider& bed3_bound_c = registry.colliders.emplace(bed3_bound);
    bed3_bound_c.type = OBSTACLE;
    bed3_bound_c.transparent = true;
    vet->addNonRenderedEntity(bed3_bound);

    // x = 270 to 390 , y = 620 to 720
    // center = 330, 770 width = 120 height = 100
    Entity table_bound = registry.create_entity();
    Motion& table_bound_motion = registry.motions.emplace(table_bound);
    table_bound_motion.position = {x_0 + 330, y_0 + 670};
    BoundingBox& table_bound_bb = registry.boundingBoxes.emplace(table_bound);
    table_bound_bb.width = 130;
    table_bound_bb.height = 100;
    Collider& table_bound_c = registry.colliders.emplace(table_bound);
    table_bound_c.type = OBSTACLE;
    table_bound_c.transparent = true;
    vet->addNonRenderedEntity(table_bound);

    // x = 290 to 330 , y = 620 to 650
    // center = 310, 635 width = 40 height = 30
    Entity single_chair_bound = registry.create_entity();
    Motion& single_chair_bound_motion = registry.motions.emplace(single_chair_bound);
    single_chair_bound_motion.position = {x_0 + 310, y_0 + 635};
    BoundingBox& single_chair_bound_bb = registry.boundingBoxes.emplace(single_chair_bound);
    single_chair_bound_bb.width = 40;
    single_chair_bound_bb.height = 50;
    Collider& single_chair_bound_c = registry.colliders.emplace(single_chair_bound);
    single_chair_bound_c.type = OBSTACLE;
    single_chair_bound_c.transparent = true;
    vet->addNonRenderedEntity(single_chair_bound);


    // BOUNCER NPC ENTITY
    Entity bouncer = registry.create_entity();
    Motion& bouncer_motion = registry.motions.emplace(bouncer);
    // bouncer_motion.position = {centered_x + (scaled_tile_width * 15.6), centered_y + (scaled_tile_height *6.2)};
    bouncer_motion.position = {center_x, center_y};
    bouncer_motion.scale = {scaled_tile_width * 0.8, scaled_tile_height*1.2};
    bouncer_motion.velocity = {0, 0};

    Collider& bouncer_collider = registry.colliders.emplace(bouncer);
    bouncer_collider.type = CREATURE;

    BoundingBox& bouncer_bb = registry.boundingBoxes.emplace(bouncer);
    bouncer_bb.width = scaled_tile_width;
    bouncer_bb.height = scaled_tile_height;

    Stats& bouncer_stats = registry.stats.emplace(bouncer);
    bouncer_stats.maxHp = 55;
    bouncer_stats.currentHp = 55;
    bouncer_stats.reputation = 80;
    bouncer_stats.cuteness = 55;
    bouncer_stats.ferocity = 60;

    NPC& bouncer_npc = registry.npcs.emplace(bouncer);
    bouncer_npc.encounter_texture_id = (int)TEXTURE_ASSET_ID::TEMP_NPC;
    bouncer_npc.name = "Thug 2";
    bouncer_npc.isInteractable = true;
    bouncer_npc.dropKey = true;

    bouncer_npc.dialogue.push("Thug: You think you can scare me, little cat?");
    bouncer_npc.dialogue.push("Thug: I'm not scared of anything!");

    bouncer_npc.attackDuration = 10.0f;

    vet->addNonRenderedEntity(bouncer);
    vet->addRenderedEntity(bouncer);
    RenderRequest bouncer_request = {
        TEXTURE_ASSET_ID::BOUNCER_LEFT,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };
    int bouncer_id = bouncer;
    vet->entity_render_requests[bouncer_id] = bouncer_request;

    Entity bouncer_encounter_icon = registry.create_entity();
    Motion& bouncer_icon_motion = registry.motions.emplace(bouncer_encounter_icon);

    bouncer_icon_motion.position = {bouncer_motion.position.x, bouncer_motion.position.y - scaled_tile_height*0.4};
    bouncer_icon_motion.scale = {scaled_tile_width * 0.6, scaled_tile_height * 0.6};
    bouncer_icon_motion.velocity = {0, 0};
    bouncer_icon_motion.z = 1;

    registry.uiElements.emplace(bouncer_encounter_icon);
    
    vet->addRenderedEntity(bouncer_encounter_icon);
    RenderRequest bouncer_icon_request = {
        TEXTURE_ASSET_ID::ENCOUNTER_ICON,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    bouncer_npc.interactIcon = bouncer_encounter_icon;
    int bouncer_encounter_icon_id = bouncer_encounter_icon;
    vet->entity_render_requests[bouncer_encounter_icon_id] = bouncer_icon_request;

    Stats med_kit_stats;
    med_kit_stats.maxHp = 5;
    med_kit_stats.currentHp = 10;
    Entity med_kit = createConsumable({ scaled_tile_width * 10.0f,scaled_tile_height * 4.0f }, med_kit_stats, TEXTURE_ASSET_ID::MEDICAL_KIT);
    vet->addRenderedEntity(med_kit);

    RenderRequest med_kit_render_request = {
        TEXTURE_ASSET_ID::MEDICAL_KIT,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    vet->entity_render_requests[med_kit] = med_kit_render_request;
    level->addRoom(vet);


}

void createBossRoom(Level* level,RenderSystem* renderer) {
    Room* boss = new Room("boss room");

    boss_to_lobby_id = registry.create_entity();

    boss->player_position = {window_width/2 - 350, window_height/2 - 240};
    boss->player_scale = { scaled_tile_width * 1.2, scaled_tile_height * 1.2 };

    Entity background = registry.create_entity();
    RenderRequest background_png = {
        TEXTURE_ASSET_ID::BOSS_ROOM,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE,
        {},
        false
    };

    Motion& background_motion = registry.motions.emplace(background);

    background_motion.scale = {1024, 1024};
    float y_scale = window_height/2;
    if (y_scale/2 < 512)
    {
        y_scale += (512 - window_height/2);
    }

    background_motion.position = {window_width/2, y_scale};

    boss->entity_render_requests[background] = background_png;
    
    boss->addRenderedEntity(background);

    boss->gridDimensions = {15, 15};

    Entity boss_player = registry.players.entities[0];
    int boss_player_id = boss_player;
    RenderRequest boss_player_request;
    boss_player_request.used_texture = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    boss_player_request.used_effect = EFFECT_ASSET_ID::TEXTURED;
    boss_player_request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    boss_player_request.hasAnimation = true;
    renderer->setAnimation(boss_player_request, AnimationState::MOVING_RIGHT, renderer->catAnimationMap);

    boss->entity_render_requests[boss_player_id] = boss_player_request;


    boss->addRenderedEntity(boss_player);
    boss->addNonRenderedEntity(boss_player);


    // Bounding Boxes

    // We need to use relative position using window_width/2 and window_height/2 as our center

    int center_x = window_width / 2;
    int center_y = y_scale;


    // the 0, 0 relative to window is center_x - 512, center_y - 512
    int x_0 = center_x - 512;
    int y_0 = center_y - 512;



    // Set room width and height
    boss->room_width = background_motion.scale.x + x_0;
    boss->room_height = background_motion.scale.y + y_0;

    Entity boss_lobby_exit = registry.create_entity();
    Motion& boss_lobby_motion = registry.motions.emplace(boss_lobby_exit);
    registry.doors.emplace(boss_lobby_exit);
    boss_lobby_motion.position = {x_0 + 35, y_0 + 160};

    Collider& boss_lobby_collider = registry.colliders.emplace(boss_lobby_exit);
    boss_lobby_collider.type = DOOR;

    BoundingBox& boss_lobby_bb = registry.boundingBoxes.emplace(boss_lobby_exit);
    boss_lobby_bb.width = 70;
    boss_lobby_bb.height = 100;

    boss->addNonRenderedEntity(boss_lobby_exit);
    boss_to_lobby_id = boss_lobby_exit;
    level->spawnPositions[boss_to_lobby_id] =  {window_width/2 - 350, window_height/2 - 240};


    Entity bottom_bound = registry.create_entity();
    Motion& bottom_motion = registry.motions.emplace(bottom_bound);
    bottom_motion.position = {center_x, center_y + 512};
    BoundingBox& bottom_bb = registry.boundingBoxes.emplace(bottom_bound);
    bottom_bb.height = 80;
    bottom_bb.width = 1024;
    Collider& bottom_c = registry.colliders.emplace(bottom_bound);
    bottom_c.type = OBSTACLE;
    boss->addNonRenderedEntity(bottom_bound);

    Entity top_bound = registry.create_entity();
    Motion& top_motion = registry.motions.emplace(top_bound);
    top_motion.position = {center_x, center_y - 512};
    BoundingBox& top_bb = registry.boundingBoxes.emplace(top_bound);
    top_bb.height = 40;
    top_bb.width = 1024;
    Collider& top_c = registry.colliders.emplace(top_bound);
    top_c.type = OBSTACLE;
    boss->addNonRenderedEntity(top_bound);


    Entity left_bound = registry.create_entity();
    Motion& left_motion = registry.motions.emplace(left_bound);
    left_motion.position = {center_x - 512, center_y};
    BoundingBox& left_bb = registry.boundingBoxes.emplace(left_bound);
    left_bb.height = 1024;
    left_bb.width = 40;
    Collider& left_c = registry.colliders.emplace(left_bound);
    left_c.type = OBSTACLE;
    boss->addNonRenderedEntity(left_bound);


    Entity right_bound = registry.create_entity();
    Motion& right_motion = registry.motions.emplace(right_bound);
    right_motion.position = {center_x + 512, center_y};
    BoundingBox& right_bb = registry.boundingBoxes.emplace(right_bound);
    right_bb.height = 1024;
    right_bb.width = 40;
    Collider& right_c = registry.colliders.emplace(right_bound);
    right_c.type = OBSTACLE;
    boss->addNonRenderedEntity(right_bound);

    // Entity patrol1 = createPatrol({x_0 + 400, y_0 + 200}, 0, scaled_tile_width, scaled_tile_height);
    // addPatrolToRoom(renderer, boss, patrol1);

    // Entity patrol2 = createPatrol({x_0 + 400, y_0 + 600}, 0, scaled_tile_width, scaled_tile_height);
    // addPatrolToRoom(renderer, boss, patrol2);

    // Entity patrol3 = createPatrol({x_0 + 200, y_0 + 800}, 0, scaled_tile_width, scaled_tile_height);
    // addPatrolToRoom(renderer, boss, patrol3);

    // Entity patrol4 = createPatrol({x_0 + 300, y_0 + 500}, 0, scaled_tile_width, scaled_tile_height);
    // addPatrolToRoom(renderer, boss, patrol4);

    Stats big_boss_stats;
    // TODO: figure out a reasonable value for these stats
    big_boss_stats.maxHp = 80;
    big_boss_stats.currentHp = 80;
    big_boss_stats.reputation = 80;
    big_boss_stats.cuteness = 75;
    big_boss_stats.ferocity = 90;

    Entity big_boss = createNPC({x_0 + 500, y_0 + 500}, scaled_tile_width * 1.5, scaled_tile_height * 1.5, big_boss_stats, "Big Boss", true, 15.0f, TEXTURE_ASSET_ID::BIG_BOSS_ATTACK);
    registry.npcs.get(big_boss).dialogue.push("Big Boss: Prepare to be euthanize!");
    
    addNPCToRoom(renderer, boss, big_boss, TEXTURE_ASSET_ID::BIG_BOSS, AnimationState::IDLE, renderer->npcAnimationMap);

    level->addRoom(boss);
}

void createTestLevel(RenderSystem* renderer, Level* level, Entity player) {
    player_id = player.getId();
    glfwGetWindowSize(renderer->getWindow(), &window_width, &window_height);

    createKennelRoom(level, renderer);
    createTunnelRoom(renderer, level);
    createCityMap(level, renderer);
    createLobbyRoom(level, renderer);
    createOfficeRoom(level, renderer);
    createVetRoom(level, renderer);
    createBossRoom(level, renderer);

    level->connectedRooms[base_to_tunnel_id] = level->rooms[1];
    level->connectedRooms[tunnel_to_base_id] = level->rooms[0];
    level->connectedRooms[tunnel_to_map_id] = level->rooms[2];
    level->connectedRooms[map_to_lobby_id] = level->rooms[3];
    level->connectedRooms[lobby_to_map_id] = level->rooms[2];
    level->connectedRooms[map_to_office_id] = level->rooms[4];
    level->connectedRooms[office_to_map_id] = level->rooms[2];
    level->connectedRooms[map_to_vet_id] = level->rooms[5];
    level->connectedRooms[vet_to_map_id] = level->rooms[2];
    level->connectedRooms[lobby_to_boss_id] = level->rooms[6];
    level->connectedRooms[boss_to_lobby_id] = level->rooms[3];
    level->spawnDirections[base_to_tunnel_id] = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    level->spawnDirections[tunnel_to_base_id] = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    level->spawnDirections[tunnel_to_map_id] = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    level->spawnDirections[lobby_to_map_id] = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    level->spawnDirections[map_to_lobby_id] = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    level->spawnDirections[map_to_office_id] = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    level->spawnDirections[office_to_map_id] = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    level->spawnDirections[map_to_vet_id] = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    level->spawnDirections[vet_to_map_id] = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    level->spawnDirections[lobby_to_boss_id] = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
    level->spawnDirections[boss_to_lobby_id] = TEXTURE_ASSET_ID::BLACK_CAT_SPRITE_SHEET;
}

void createLevels(RenderSystem* renderer, LevelSystem* levelManager, Entity player) {
    Level* testLevel = new Level("Test Level");
    createTestLevel(renderer, testLevel, player);
    levelManager->addLevel(testLevel);
}