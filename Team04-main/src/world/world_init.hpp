#pragma once

#include "../common.hpp"
#include "core/ecs.hpp"
#include "systems/render_system.hpp"
#include "systems/levels_rooms_system.hpp"

// the player
const float PLAYER_WIDTH = 3 * 32.f;
const float PLAYER_HEIGHT = 3 * 32.f;

const float ITEM_WIDTH = 2 * 16.f;
const float ITEM_HEIGHT = 2 * 16.f;

const float PATROL_WIDTH = 3 * 32.f;
const float PATROL_HEIGHT = 3 * 32.f;

const float CONE_WIDTH = 3 * 64.f;
const float CONE_HEIGHT = 3 * 64.f;

Entity createPlayer(vec2 position, float tile_width, float tile_height);
std::pair<Entity, Entity> createBackpack(RenderSystem* renderer, vec2 pos);
Entity createPatrol(vec2 position, float ms_to_turn, float tile_width, float tile_height);
Entity createConsumable(vec2 position, Stats stats, TEXTURE_ASSET_ID texture);
Entity createEquippable(vec2 position, Stats stats, TEXTURE_ASSET_ID texture);

void createTestLevel(RenderSystem* renderer, Level* level, Entity player);
void createLevels(RenderSystem* renderer, LevelSystem* levelManager, Entity player);
void createCityMap(Level* level, RenderSystem* renderer);
void createOfficeRoom(Level* level, RenderSystem* renderer);
void createVetRoom(Level* level, RenderSystem* renderer);
void createBossRoom(Level* level, RenderSystem* renderer);