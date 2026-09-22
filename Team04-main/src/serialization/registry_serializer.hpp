#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "core/ecs_registry.hpp"
#include "../src/systems/levels_rooms_system.hpp"
#include "../src/world/world_system.hpp"
#include "systems/render_system.hpp"

using json = nlohmann::json;

class RegistrySerializer {
public:
    static const std::string SAVE_LOAD_DIR;
    static std::string SAVE_LOAD_FILE_NAME;
    // Save the ECS registry state to a JSON file
    static void saveRegistryState(const std::string& stateName, WorldSystem* worldSystem);

    // Load the ECS registry state from a JSON file
    static bool loadRegistryState(const std::string& stateName, WorldSystem* game);

    static bool updateRoomRenderRequests(WorldSystem *game, int oldId, RenderRequest rr);

    static bool updateRooms(WorldSystem* game, Entity entity, int oldId);

    static bool updateLevels(WorldSystem* game, Entity entity, int oldId);

    static void cleanupSavedRegistry();

    static void cleanupStateFile(const std::string & stateName);

    static void cleanupSaveLoadDirectory();

    static bool encounterUpdateStateFile(Stats player_stats, Stats npc_stats, int player_index);

private:
    // Create Directory if it doesn't exist
    // static void createDirectoryIfNotExists(const std::string& dirPath);
    
    // SERIALIZERS
    static json serializeMotion(const Motion& motion);
    static json serializeMesh(GEOMETRY_BUFFER_ID mesh_id);
    static json serializeStats(const Stats& stats);
    static json serializeConsumableItem(const ConsumableItem& item);
    static json serializeBoundingBox(const BoundingBox& boundingBox);
    static json serializeCollider(const Collider& collider);
    static json serializePatrol(const Patrol& patrol);
    static json serializeSetMotion(const SetMotion& sm);
    static json serializeHidden(const Hidden& hidden);
    static json serializeRenderRequest(const RenderRequest& renderRequest);
    static json serializeRandomWalker(const RandomWalker& randomWalker);
    static json serializeConverger(const Converger& converger);
    static json serializeChaser(const Chaser& chaser);
    static json serializeInventory(const Inventory& inventory);
    static json serializeEquippableItem(const EquippableItem& equippableItem);
    static json serializeRoom(const Room* room);
    static json serializeLevel(const Level* room);
    static json serializeLevelSystem(const LevelSystem* levelSystem);
    static json serializeNPC(const NPC& npc);
    static json serializeDoor(const Door& door);
    static json serializeTime(const Time& time);
    static json serializeKey(const Key& key);
    static json serializeKeyInventory(const KeyInventory& KeyInventory);
    static json serializePlayer(const Player& player);

    // DESERIALIZERS
    static Inventory      deserializeInventory(const json& inventory_json);
    static Stats          deserializeStats(const json& stats_json);
    static ConsumableItem deserializeConsumableItem(const json& item_json);
    static EquippableItem deserializeEquippableItem(const json& equippableItem_json);
    static Room*          deserializeRoom(const json& j, std::unordered_map<unsigned int, Entity>& idMap);
    static Level*         deserializeLevel(const json& level, std::unordered_map<unsigned int, Entity>& idMap);
    static RenderRequest  deserializeRenderRequest(const json& rrJson);
    static LevelSystem*   deserializeLevelSystem(const json& j, std::unordered_map<unsigned int, Entity>& idMap);
    static void           deserializeMesh(const json& j, Entity entity, RenderSystem& renderSystem);
    static NPC            deserializeNPC(const json& npc_json, std::unordered_map<unsigned int, Entity>& idMap);
    static Door           deserializeDoor(const json& door_json);
    static Time           deserializeTime(const json &tJson);
    static Key            deserializeKey(const json& key_json);
    static KeyInventory   deserializeKeyInventory(const json& keyInven_json);
    static Player deserializePlayer(const json& player_json);
};