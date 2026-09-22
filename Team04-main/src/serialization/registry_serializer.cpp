#include "registry_serializer.hpp"
#include <fstream>
#include "core/components.hpp"
#include "common.hpp"
#include "world/world_init.hpp"
#include <iostream>
#include <string>
#include <vector>
#include "../src/systems/levels_rooms_system.hpp"
#include "systems/render_system.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

extern ECSRegistry registry;

const std::string RegistrySerializer::SAVE_LOAD_DIR = std::string(PROJECT_SOURCE_DIR) + "data/json/";
std::string RegistrySerializer::SAVE_LOAD_FILE_NAME = "PlayState.json";
std::unordered_map<unsigned int, Entity> idMap;

bool RegistrySerializer::encounterUpdateStateFile(Stats player_stats, Stats npc_stats, int player_index) {
    std::string file_path = save_path();

    std::ifstream in_file(file_path);
    if (!in_file.good()) {
        // printf("File %s does not exist.\n", file_path.c_str());
        return false;
    }

    // Load JSON from file.
    json j;
    in_file >> j;
    in_file.close();

    // printf("Updating registry state to: %s\n", file_path.c_str());

    j["entities"][player_index]["Stats"] = serializeStats(player_stats);

    // Write JSON to file
    std::ofstream out_file(file_path);

    if (out_file.is_open()) {
        out_file << j.dump(4); // Pretty print with indentation
        if (!out_file.good()) {
            std::cerr << "Failed to write to file " << file_path << std::endl;
            assert(false);
        }
        out_file.close();
    } else {
        std::cerr << "Error opening file for saving: " << file_path << std::endl;
    }

    return true;
}

// Save the entire ECSRegistry state to a JSON file
void RegistrySerializer::saveRegistryState(const std::string& stateName, WorldSystem* worldSystem) {
    // std::string file_path = SAVE_LOAD_DIR + stateName + ".json";
    // we are only saving the play state for now because there is no important information in the other states to be restored
    std::string file_path = save_path();
    json j;

    j["level_system"] = serializeLevelSystem(worldSystem->get_level_manager());

    for (Entity entity : registry.get_entities()) {
        json entity_json;

        entity_json["id"] = entity.getId();

        // if (registry.uiElements.has(entity) && !registry.inventory.has(entity)) {
        //     continue;
        // }

        if (registry.rotatables.has(entity))
        {
            entity_json["Rotatable"] = {};
        }

        if (registry.players.has(entity)) {
            // printf("Saving player component for entity %u\n", entity);
            entity_json["Player"] = serializePlayer(registry.players.get(entity));;
            // Set player velocity to 0
            registry.motions.get(entity).velocity = vec2(0,0);
        }

        if (registry.consumableItems.has(entity)) {
            entity_json["ConsumableItem"] = serializeConsumableItem(registry.consumableItems.get(entity));
        }

        if (registry.equippableItems.has(entity)) {
            entity_json["EquippableItem"] = serializeEquippableItem(registry.equippableItems.get(entity));
        }

        if (registry.npcs.has(entity)) {
            entity_json["NPC"] = serializeNPC(registry.npcs.get(entity));
        }

        if (registry.motions.has(entity)) {
            entity_json["Motion"] = serializeMotion(registry.motions.get(entity));
        }



        //if (registry.meshPtrs.has(entity)) {
        //    Mesh* mesh = registry.meshPtrs.get(entity);
        //    entity_json["Mesh"] = serializeMesh("/data/meshes/backpack4.obj");
        //    //Mesh* mesh_ptr = registry.meshPtrs.get(entity);
        //    //std::string str_mesh_ptr = std::to_string(reinterpret_cast<std::uintptr_t>(mesh_ptr));
        //    //entity_json["meshPtr"] = str_mesh_ptr;
        //}

        if (registry.meshPtrs.has(entity)) {
            Mesh* mesh_ptr = registry.meshPtrs.get(entity);
            GEOMETRY_BUFFER_ID mesh_id = GEOMETRY_BUFFER_ID::BACKPACK;

            for (const auto& pair : worldSystem->get_renderer()->mesh_paths) {
                const GEOMETRY_BUFFER_ID& id = pair.first;
                const std::string& path = pair.second;

                if (&worldSystem->get_renderer()->getMesh(id) == mesh_ptr) {
                    mesh_id = id;
                    break;
                }
            }
            entity_json["Mesh"] = serializeMesh(mesh_id);
        }

        if (registry.stats.has(entity)) {
            entity_json["Stats"] = serializeStats(registry.stats.get(entity));
        }

        if (registry.boundingBoxes.has(entity)) {
            entity_json["BoundingBox"] = serializeBoundingBox(registry.boundingBoxes.get(entity));
        }

        if (registry.colliders.has(entity)) {
            entity_json["Collider"] = serializeCollider(registry.colliders.get(entity));
        }

        if (registry.patrols.has(entity)) {
            entity_json["Patrol"] = serializePatrol(registry.patrols.get(entity));
        }

        if (registry.hiddens.has(entity)) {
            entity_json["Hidden"] = serializeHidden(registry.hiddens.get(entity));
        }

        if (registry.renderRequests.has(entity)) {
            entity_json["RenderRequest"] = serializeRenderRequest(registry.renderRequests.get(entity));
        }

        if (registry.randomWalkers.has(entity)) {
            entity_json["RandomWalker"] = serializeRandomWalker(registry.randomWalkers.get(entity));
        }

        if (registry.setMotions.has(entity))
        {
            entity_json["SetMotion"] = serializeSetMotion(registry.setMotions.get(entity));
        }
        

        if (registry.convergers.has(entity)) {
            entity_json["Converger"] = serializeConverger(registry.convergers.get(entity));
        }

        if (registry.chasers.has(entity)) {
            entity_json["Chaser"] = serializeChaser(registry.chasers.get(entity));
        }

        if (registry.inventory.has(entity)) {
            entity_json["Inventory"] = serializeInventory(registry.inventory.get(entity));
        }

        if (registry.doors.has(entity))
        {
            entity_json["Door"] = serializeDoor(registry.doors.get(entity));
        }
        
        if (registry.times.has(entity))
        {
            entity_json["Time"] = serializeTime(registry.times.get(entity));
        }

        if(registry.key.has(entity))
        {
            entity_json["Key"] = serializeKey(registry.key.get(entity));
        }

        if (registry.keyInventory.has(entity)) {
            entity_json["KeyInventory"] = serializeKeyInventory(registry.keyInventory.get(entity));
        }
        j["entities"].push_back(entity_json);
    }

    // Write JSON to file
    std::ofstream file(file_path);

    if (file.is_open()) {
        file << j.dump(4); // Pretty print with indentation
        if (!file.good()) {
            std::cerr << "Failed to write to file " << file_path << std::endl;
            assert(false);
        }
        file.close();
        // std::cout << "Registry state saved to " << file_path << std::endl;
    } else {
        std::cerr << "Error opening file for saving: " << file_path << std::endl;
    }
}

// Load the ECSRegistry state from a JSON file
bool RegistrySerializer::loadRegistryState(const std::string& stateName, WorldSystem* game) {
    //std::string file_path = SAVE_LOAD_DIR + stateName + ".json";
    std::string file_path = save_path();

    std::ifstream file(file_path);
    if (!file.good()) {
        // printf("File %s does not exist.\n", file_path.c_str());
        return false;
    }

    json j;
    file >> j;
    file.close();

    for (const auto& entity_json : j["entities"]) {
        unsigned int oldId = entity_json["id"];
        Entity newEntity = registry.create_entity();
        idMap[oldId] = newEntity;
    }
    LevelSystem* levelSystem = deserializeLevelSystem(j["level_system"], idMap);
    game->set_level_manager(levelSystem);
    for (const auto& entity_json : j["entities"]) {
        Entity entity = idMap[entity_json["id"]];

        // Entity entity = registry.create_entity();

        // Load Player component
        if (entity_json.contains("Player")) {
            // printf("Loading Player component for entity %u\n", entity);
            Player player = deserializePlayer(entity_json["Player"]);
            registry.players.emplace(entity, player);
        }

        // Load ConsumableItem component
        if (entity_json.contains("ConsumableItem")) {
            json consumable_json = entity_json["ConsumableItem"];
            ConsumableItem item = deserializeConsumableItem(consumable_json);
            registry.consumableItems.emplace(entity, item);
        }

        // Load EquippableItems component
        if (entity_json.contains("EquippableItem")) {
            EquippableItem item = deserializeEquippableItem(entity_json["EquippableItem"]);
            registry.equippableItems.emplace(entity, item);
        }

        if (entity_json.contains("Rotatable"))
        {
            registry.rotatables.emplace(entity);
        }

        // Load Motion component
        if (entity_json.contains("Motion")) {
            // printf("Loading Motion component for entity %u\n", entity);
            json motion_json = entity_json["Motion"];
            Motion motion;
            motion.position = {motion_json["position"][0], motion_json["position"][1]};
            motion.angle = motion_json["angle"];
            motion.velocity = {motion_json["velocity"][0], motion_json["velocity"][1]};
            motion.scale = {motion_json["scale"][0], motion_json["scale"][1]};
            motion.speed = motion_json["speed"];
            motion.speedMod = motion_json["speedMod"];
            motion.z = motion_json["z"];
            registry.motions.emplace(entity, motion);

            if(entity_json.contains("Player")) {
                game->get_level_manager()->currentLevel->currentRoom->player_position = motion.position;
            }
        }

        // Load Stats component
        if (entity_json.contains("Stats")) {
            json stats_json = entity_json["Stats"];
            Stats stats = deserializeStats(stats_json);
            registry.stats.emplace(entity, stats);
        }

        // Load Bounding_Box component
        if (entity_json.contains("BoundingBox")) {
            // printf("Loading Bounding_Box component for entity %u\n", entity);
            json bb_json = entity_json["BoundingBox"];
            BoundingBox bb;
            bb.width = bb_json["width"];
            bb.height = bb_json["height"];
            bb.offset = {bb_json["offset"][0], bb_json["offset"][1]};
            registry.boundingBoxes.emplace(entity, bb);
        }

        // Load Collider component
        if (entity_json.contains("Collider")) {
            // printf("Loading Collider component for entity %u\n", entity);
            json collider_json = entity_json["Collider"];
            Collider collider;
            collider.type = static_cast<COLLIDER_TYPE>(collider_json["type"]);
            collider.transparent = static_cast<bool>(collider_json["transparent"]);
            collider.friction = static_cast<float>(collider_json["friction"]);
            registry.colliders.emplace(entity, collider);
        }

        // Load Patrol component
        if (entity_json.contains("Patrol")) {
            // printf("Loading Patrol component for entity %u\n", entity);
            json patrol_json = entity_json["Patrol"];
            Patrol patrol;
            patrol.player_seen = patrol_json["player_seen"];
            patrol.light = idMap[static_cast<int>(patrol_json["light"])];
            registry.patrols.emplace(entity, patrol);
        }

        if (entity_json.contains("SetMotion"))
        {
            json sm_json = entity_json["SetMotion"];
            SetMotion sm;
            sm.timer = sm_json["timer"];
            registry.setMotions.emplace(entity, sm);
        }
        

        //// Load RenderRequest component
        //if (entity_json.contains("RenderRequest")) {
        //    json rr_json = entity_json["RenderRequest"];
        //    RenderRequest rr = deserializeRenderRequest(rr_json);
        //    registry.renderRequests.insert(entity, rr);

        //}

        // Load RandomWalker component
        if (entity_json.contains("RandomWalker")) {
            // printf("Loading RandomWalker component for entity %u\n", entity);
            json rw_json = entity_json["RandomWalker"];
            RandomWalker rw;
            rw.sec_since_turn = rw_json["sec_since_turn"];
            rw.sec_to_turn = rw_json["sec_to_turn"];
            registry.randomWalkers.emplace(entity, rw);
        }

        // Load Converger component
        if (entity_json.contains("Converger")) {
            json converger_json = entity_json["Converger"];
            Converger c;
            c.target_pos = {converger_json["target_pos"][0], converger_json["target_pos"][1]};
            registry.convergers.emplace(entity, c);
        }

        // Load Chaser component
        if (entity_json.contains("Chaser")) {
            json chaser_json = entity_json["Chaser"];
            Chaser ch;
            ch.target_pos = {chaser_json["target_pos"][0], chaser_json["target_pos"][1]};
            registry.chasers.emplace(entity, ch);
        }

        // Load Hidden component
        if (entity_json.contains("Hidden")) {
            json hidden_json = entity_json["Hidden"];
            Hidden h;
            h.hidden = hidden_json["Hidden"];
            registry.hiddens.emplace(entity, h);
        }

        // Load Mesh pointer component
        if (entity_json.contains("Mesh")) {
            deserializeMesh(entity_json["Mesh"], entity, *game->get_renderer());

        }

        // Load Inventory
        if (entity_json.contains("Inventory")) {
            json inventory_json = entity_json["Inventory"];
            Inventory inventory = deserializeInventory(inventory_json);
            registry.inventory.emplace(entity, inventory);
        }

        // Load Door
        if (entity_json.contains("Door")) {
            json door_json = entity_json["Door"];
            Door door = deserializeDoor(door_json);
            registry.doors.emplace(entity, door);
        }

        // Load NPC
        if (entity_json.contains("NPC"))
        {
            json npc_json = entity_json["NPC"];
            NPC npc = deserializeNPC(npc_json, idMap);
            registry.npcs.emplace(entity, npc);
        }

        if(entity_json.contains("Key"))
        {
            json key_json = entity_json["Key"];
            Key key = deserializeKey(key_json);
            registry.key.emplace(entity, key); 
        }

        if (entity_json.contains("KeyInventory")) 
        {
            KeyInventory keyInventory = deserializeKeyInventory(entity_json["KeyInventory"]);
            registry.keyInventory.emplace(entity, keyInventory);
        }

        // Load Time
        if (entity_json.contains("Time"))
        {
            json time_json = entity_json["Time"];
            Time time = deserializeTime(time_json);
            registry.times.emplace(entity, time);
        }
    }

    return true;
}

// VINCENT'S CODE IS UNCOMMENTED, NICO'S IS COMMENTED OUT
bool RegistrySerializer::updateRoomRenderRequests(WorldSystem *game, int oldId, RenderRequest rr) {
    for (Level* level: game->get_level_manager()->levels) {
        for(Room* room: level->rooms) {
            /*
            // Find RenderRequest by Entity ID
            auto it = room->entity_render_requests.find(oldId);
            if (it != room->entity_render_requests.end()) {
                // Insert the RenderRequest with the new ID
                int newId = idMap[oldId];
                room->entity_render_requests[newId] = rr;

                // Erase the old ID entry
                room->entity_render_requests.erase(it);
                return true;
            }*/


            for(const auto& entry : room->entity_render_requests) {
                    if(idMap.find(oldId) != idMap.end()) {
                        int newId = idMap[oldId];
                        room->entity_render_requests[newId] = rr;
                        return true;
                    }
                }
            }
        }
    return false;
}


bool RegistrySerializer::updateRooms(WorldSystem* game, Entity entity, int oldId) {
    for (Level * level : game->get_level_manager()->levels) {
        for (Room * room : level->rooms) {

            // NON_RENDERED ENTITIES
            auto it_non_rendered_e = std::find_if(room->non_rendered_entities.begin(),
                room->non_rendered_entities.end(),
                [oldId](const Entity& e) { return e.getId() == oldId; });
                // replace
            if (it_non_rendered_e != room->non_rendered_entities.end()) {
                *it_non_rendered_e = entity;
            }

            // RENDERED ENTITIES
            auto it_rendered_e = std::find_if(room->rendered_entities.begin(),
                room->rendered_entities.end(),
                [oldId](const Entity& e) { return e.getId() == oldId; });
            // replace
            if (it_rendered_e != room->rendered_entities.end()) {
                *it_rendered_e = entity;
            }

            // NICO'S ORIGINAL CODE
            // For Entity Render Request Map
            auto it_e_rr = room->entity_render_requests.find(oldId);
            if (it_e_rr != room->entity_render_requests.end()) {
                // Insert the RenderRequest with the new ID
                room->entity_render_requests[int(entity)] = it_e_rr->second;

                room->entity_render_requests.erase(it_e_rr); // Erase the old entry
                // Remove from renderRequests
                registry.renderRequests.remove(entity);
            }
        }
    }
    return true;
}

bool RegistrySerializer::updateLevels(WorldSystem* game, Entity entity, int oldId) {
    for (Level * level : game->get_level_manager()->levels) {
        // Connected Rooms
        auto it_cr = level->connectedRooms.find(oldId);
        if (it_cr != level->connectedRooms.end()) {
            Room* room = it_cr->second;
            level->connectedRooms.erase(it_cr);  // Remove the old entry
            level->connectedRooms[(int)entity] = room;  // Insert the new entry
        }

        // Spawn Positions
        auto it_sp = level->spawnPositions.find(oldId);
        if (it_sp != level->spawnPositions.end()) {
            vec2 position = it_sp->second;
            level->spawnPositions.erase(it_sp);  // Remove the old entry
            level->spawnPositions[(int)entity] = position;  // Insert the new entry
        }

        // Spawn Directions
        auto it_sd = level->spawnDirections.find(oldId);
        if (it_sd != level->spawnDirections.end()) {
            TEXTURE_ASSET_ID direction = it_sd->second;
            level->spawnDirections.erase(it_sd);  // Remove the old entry
            level->spawnDirections[(int)entity] = direction;  // Insert the new entry
        }
    }
    return true;
}

void RegistrySerializer::cleanupSavedRegistry() {
    std::string directoryPath = save_path_dir();

#ifdef _WIN32
    // Windows-specific directory iteration
    std::string searchPath = directoryPath + "*.json";
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Could not open directory: " << directoryPath << std::endl;
        return;
    }

    do {
        std::string filePath = directoryPath + findFileData.cFileName;
        if (remove(filePath.c_str()) == 0) {
            //std::cout << "Deleted file: " << filePath << std::endl;
        } else {
            std::cerr << "Error deleting file: " << filePath << std::endl;
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

#else
    // POSIX-specific directory iteration
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(directoryPath.c_str())) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            std::string fileName = ent->d_name;
            if (fileName.size() > 5 && fileName.substr(fileName.size() - 5) == ".json") {
                std::string filePath = directoryPath + fileName;
                if (remove(filePath.c_str()) == 0) {
                    // std::cout << "Deleted file: " << filePath << std::endl;
                } else {
                    std::cerr << "Error deleting file: " << filePath << std::endl;
                }
            }
        }
        closedir(dir);
    } else {
        perror("Could not open directory");
    }
#endif
}

void RegistrySerializer::cleanupStateFile(const std::string& stateName) {
    // for now we are only saving PlayState because class name casting doesn't work the same for windows and mac.
    // we can improve this in the future
    std::string file_path = save_path();

    // Check if the file exists before attempting to delete
    std::ifstream file(file_path.c_str());
    if (file) {
        file.close();
        if (remove(file_path.c_str()) == 0) {
            //std::cout << "Deleted file: " << file_path << std::endl;
        } else {
            std::cerr << "Error deleting file: " << file_path << std::endl;
        }
    } else {
        // std::cout << "File does not exist, skipping delete: " << file_path << std::endl;
    }
}

void RegistrySerializer::cleanupSaveLoadDirectory() {
    std::string dir_path = save_path_dir();

#ifdef _WIN32
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((dir_path + "\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Could not open directory: " << dir_path << std::endl;
        return;
    }

    do {
        std::string file_name = findFileData.cFileName;

        // Skip the special entries "." and ".."
        if (file_name == "." || file_name == "..") {
            continue;
        }

        std::string file_path = dir_path + "\\" + file_name;

        // Attempt to delete the file
        if (DeleteFile(file_path.c_str())) {
            //std::cout << "Deleted file: " << file_path << std::endl;
        } else {
            std::cerr << "Error deleting file: " << file_path << std::endl;
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

#else
    DIR* dir = opendir(dir_path.c_str());
    if (dir == nullptr) {
        std::cerr << "Error: Could not open directory: " << dir_path << std::endl;
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string file_name = entry->d_name;

        // Skip the special entries "." and ".."
        if (file_name == "." || file_name == "..") {
            continue;
        }

        std::string file_path = dir_path + "/" + file_name;

        // Attempt to delete the file
        if (remove(file_path.c_str()) == 0) {
            // std::cout << "Deleted file: " << file_path << std::endl;
        } else {
            std::cerr << "Error deleting file: " << file_path << std::endl;
        }
    }
    closedir(dir);
#endif
}

// Serialize Motion component
json RegistrySerializer::serializeMotion(const Motion& motion) {
    return {
        {"position", {motion.position.x, motion.position.y}},
        {"angle", motion.angle},
        {"velocity", {motion.velocity.x, motion.velocity.y}},
        {"scale", {motion.scale.x, motion.scale.y}},
        {"speed", motion.speed},
        {"speedMod", motion.speedMod},
        {"z", motion.z}
    };
}

json RegistrySerializer::serializeMesh(GEOMETRY_BUFFER_ID mesh_id) {
    return {
        {"mesh_id", static_cast<int>(mesh_id)}
    };
}

void RegistrySerializer::deserializeMesh(const json& j, Entity entity, RenderSystem& renderSystem) {
    if (j.contains("mesh_id")) {
        GEOMETRY_BUFFER_ID mesh_id = static_cast<GEOMETRY_BUFFER_ID>(j["mesh_id"]);
        auto it = std::find_if(renderSystem.mesh_paths.begin(), renderSystem.mesh_paths.end(),
            [mesh_id](const std::pair<GEOMETRY_BUFFER_ID, std::string>& pair) {
                return pair.first == mesh_id;
            });

        if (it != renderSystem.mesh_paths.end()) {
            const std::string path = it->second;
            Mesh* mesh_ptr = new Mesh();
            if (Mesh::loadFromOBJFile(path, mesh_ptr->vertices, mesh_ptr->vertex_indices, mesh_ptr->original_size)) {
                registry.meshPtrs.emplace(entity, mesh_ptr);
            }
        }
    }
}

NPC RegistrySerializer::deserializeNPC(const json& npc_json, std::unordered_map<unsigned int, Entity>& idMap)
{
    NPC npc;
    npc.name = npc_json["name"];
    npc.encounter_texture_id = npc_json["encounter_texture_id"];
    npc.isInteractable = npc_json["isInteractable"];
    npc.start_encounter = npc_json["start_encounter"];
    npc.blocking_door = npc_json["blocking_door"];
    npc.blocked_door = idMap[npc_json["blocked_door"]];
    npc.hasDialogue = npc_json["hasDialogue"];
    npc.isTutorialNPC = npc_json["isTutorialNPC"];
    npc.isDefeated = npc_json["isDefeated"];
    npc.interactDistance = npc_json["interactDistance"];
    npc.dropKey = npc_json["dropKey"];
    npc.interactDistance = npc_json["interactDistance"];

    for (const auto& line : npc_json["dialogue"]) {
        npc.dialogue.push(line);
    }
    npc.to_remove = npc_json["to_remove"];

    unsigned int oldID = npc_json["interactIconId"];
    if(idMap.find(oldID) != idMap.end()) {
        Entity entity = idMap[oldID];
        npc.interactIcon = entity;
    }

    return npc;
}

Door RegistrySerializer::deserializeDoor(const json& door_json)
{
    Door door;
    door.is_open = door_json["is_open"];
    door.required_keys = door_json["required_keys"];

    return door;
}

// Serialize Stats component
json RegistrySerializer::serializeStats(const Stats& stats) {
    return {
        {"maxHp", stats.maxHp},
        {"currentHp", stats.currentHp},
        {"stamina", stats.stamina},
        {"ferocity", stats.ferocity},
        {"cuteness", stats.cuteness},
        {"stealth", stats.stealth},
        {"agility", stats.agility},
        {"reputation", stats.reputation},
        {"intelligence", stats.intelligence}
    };
}

json RegistrySerializer::serializeConsumableItem(const ConsumableItem& item) {
    return {
        {"stats", serializeStats(item.statModifiers)},
        {"is_backpack", item.isBackpack},
        {"isInteractable", item.isInteractable}
    };
}

ConsumableItem RegistrySerializer::deserializeConsumableItem(const json& item_json) {
    ConsumableItem item;
    item.isBackpack = item_json["is_backpack"];
    item.statModifiers = deserializeStats(item_json["stats"]);
    item.isInteractable = item_json["isInteractable"];
    return item;
}

Stats RegistrySerializer::deserializeStats(const json& stats_json) {
    Stats stats;
    stats.maxHp = stats_json["maxHp"];
    stats.currentHp = stats_json["currentHp"];
    stats.stamina = stats_json["stamina"];
    stats.intelligence = stats_json["intelligence"];
    stats.cuteness = stats_json["cuteness"];
    stats.ferocity = stats_json["ferocity"];
    stats.stealth = stats_json["stealth"];
    stats.agility = stats_json["agility"];
    stats.reputation = stats_json["reputation"];
    return stats;
}

// Serialize BoundingBox component
json RegistrySerializer::serializeBoundingBox(const BoundingBox& boundingBox) {
    return {
        {"width", boundingBox.width},
        {"height", boundingBox.height},
        {"offset", {boundingBox.offset.x, boundingBox.offset.y}}
    };
}

// Serialize Collider component
json RegistrySerializer::serializeCollider(const Collider& collider) {
    return {
        {"type", static_cast<int>(collider.type)},
        {"transparent", static_cast<bool>(collider.transparent)},
        {"friction", static_cast<float>(collider.friction)}
    };
}

// Serialize Patrol component
json RegistrySerializer::serializePatrol(const Patrol& patrol) {
    return {
        {"player_seen", patrol.player_seen},
        {"light", patrol.light.getId()}
    };
}

json RegistrySerializer::serializeSetMotion(const SetMotion& sm) {
    return {
        {"timer", sm.timer}
    };
}

// Serialize Hidden component
json RegistrySerializer::serializeHidden(const Hidden& hidden) {
    return {
        {"hidden", hidden.hidden}
    };
}

// Serialize RenderRequest component
json RegistrySerializer::serializeRenderRequest(const RenderRequest& renderRequest) {
    return {
        {"used_texture", static_cast<int>(renderRequest.used_texture)},
        {"used_effect", static_cast<int>(renderRequest.used_effect)},
        {"used_geometry", static_cast<int>(renderRequest.used_geometry)},
        {"hasAnimation", renderRequest.hasAnimation},
        {"animation", {
            {"frameCount", renderRequest.animation.frameCount},
            {"currentFrame", renderRequest.animation.currentFrame},
            {"frameTime", renderRequest.animation.frameTime},
            {"elapsedTime", renderRequest.animation.elapsedTime},
            {"columns", renderRequest.animation.columns},
            {"rows", renderRequest.animation.rows},
            {"startRow", renderRequest.animation.startRow},
            {"startCol", renderRequest.animation.startCol},
            {"current_state", static_cast<AnimationState>(renderRequest.animation.current_state)}
        }},
        {"alpha", renderRequest.alpha}
    };
}

// Serialize RandomWalker component
json RegistrySerializer::serializeRandomWalker(const RandomWalker& randomWalker) {
    return {
        {"sec_since_turn", randomWalker.sec_since_turn},
        {"sec_to_turn", randomWalker.sec_to_turn}
    };
}

// Serialize Converger component
json RegistrySerializer::serializeConverger(const Converger& converger) {
    return {
        {"target_pos", {converger.target_pos.x, converger.target_pos.y}}
    };
}

// Serialize Chaser component
json RegistrySerializer::serializeChaser(const Chaser& chaser) {
    return {
        {"target_pos", {chaser.target_pos.x, chaser.target_pos.y}},
    };
}

json RegistrySerializer::serializeInventory(const Inventory& inventory) {
    json inventory_json;
    for (const auto& item : inventory.items) {
        inventory_json["items"].push_back(item.getId());
    }
    return inventory_json;
}

Inventory RegistrySerializer::deserializeInventory(const json& inventory_json) {
    Inventory inventory;
    if (inventory_json.contains("items") && !inventory_json["items"].is_null()) {
        for (const auto& item_json : inventory_json["items"]) {
            unsigned int oldID = item_json;
            if (idMap.find(oldID) != idMap.end()) {
                Entity entity = idMap[oldID];
                inventory.items.push_back(entity);
            }

        }
    }
    return inventory;
}

json RegistrySerializer::serializeEquippableItem(const EquippableItem& equippableItem) {
    return {
            {"texture", static_cast<int> (equippableItem.texture)},
            {"statModifiers", serializeStats(equippableItem.statModifiers)},
            {"pickupCooldown", equippableItem.pickupCooldown},
            {"isInteractable", equippableItem.isInteractable}
    };
}

EquippableItem RegistrySerializer::deserializeEquippableItem(const json& equippableItem_json) {
    EquippableItem equippableItem;
    equippableItem.texture = equippableItem_json["texture"];
    equippableItem.statModifiers = deserializeStats(equippableItem_json["statModifiers"]);
    equippableItem.pickupCooldown = equippableItem_json["pickupCooldown"];
    equippableItem.isInteractable = equippableItem_json["isInteractable"];
    return equippableItem;
}

json RegistrySerializer::serializeRoom(const Room* room) {
    json j;
    j["name"] = room->name;

    j["rendered_entities"] = json::array();
    j["non_rendered_entities"] = json::array();

    j["room_width"] = room->room_width;
    j["room_height"] = room->room_height;

    for (const auto& entity : room->rendered_entities) {
        j["rendered_entities"].push_back(entity.getId());
    }

    for (const auto& entity : room->non_rendered_entities) {
        j["non_rendered_entities"].push_back(entity.getId());
    }
    
    j["entity_render_requests"] = json::object();
    for (const auto& pair : room->entity_render_requests) {
        int entityId = pair.first;
        const RenderRequest& renderRequest = pair.second;
        j["entity_render_requests"][std::to_string(entityId)] = serializeRenderRequest(renderRequest);
    }
    
    j["player_position"] = { room->player_position.x, room->player_position.y };
    j["player_scale"] = { room->player_scale.x, room->player_scale.y };

    j["mesh_to_texture_map"] = json::object();
    for (const auto& pair : room->meshToTextureMap) {
        j["mesh_to_texture_map"][std::to_string(pair.first.getId())] = pair.second.getId();
    }

    return j;

}

Room* RegistrySerializer::deserializeRoom(const json& j, std::unordered_map<unsigned int, Entity>& idMap) {
    Room* room = new Room(j["name"]);
    for (const auto& id : j["rendered_entities"]) {
        if (idMap.find(id) != idMap.end()) {
            room->rendered_entities.push_back(idMap[id]);
        }
    }
    for (const auto& id : j["non_rendered_entities"]) {
        if (idMap.find(id) != idMap.end()) {
            room->non_rendered_entities.push_back(idMap[id]);
        }
    }

    for (const auto& item : j["entity_render_requests"].items()) {
        unsigned int oldId = std::stoul(item.key());
        if (idMap.find(oldId) != idMap.end()) {
            room->entity_render_requests[int(idMap[oldId])] = deserializeRenderRequest(item.value());
        }
    }

    room->player_position = { j["player_position"][0], j["player_position"][1] };
    room->player_scale = { j["player_scale"][0], j["player_scale"][1] };
    room->room_height = j["room_height"];
    room->room_width = j["room_width"];

    for (const auto& item : j["mesh_to_texture_map"].items()) {
        unsigned int meshId = std::stoul(item.key());
        unsigned int textureId = item.value();
        if (idMap.find(meshId) != idMap.end() && idMap.find(textureId) != idMap.end()) {
            room->meshToTextureMap[idMap[meshId]] = idMap[textureId];
        }
    }

    return room;
}

json RegistrySerializer::serializeLevel(const Level* level) {
    json j;
    j["name"] = level->name;
    j["current_room_name"] = level->currentRoom->name;
    j["rooms"] = json::array();
    for (const Room* room : level->rooms) {
        j["rooms"].push_back(serializeRoom(room));
    }

    j["connected_rooms"] = json::object();
    for (const auto& pair : level->connectedRooms) {
        j["connected_rooms"][std::to_string(pair.first)] = pair.second->name;
    }

    j["spawn_positions"] = json::object();
    for (const auto& pair : level->spawnPositions) {
        j["spawn_positions"][std::to_string(pair.first)] = { pair.second.x, pair.second.y };
    }

    j["spawn_directions"] = json::object();
    for (const auto& pair : level->spawnDirections) {
        j["spawn_directions"][std::to_string(pair.first)] = static_cast<int>(pair.second);
    }

   

    return j;

}

Level* RegistrySerializer::deserializeLevel(const json& j, std::unordered_map<unsigned int, Entity>& idMap) {
    Level* level = new Level(j["name"]);


    for (const auto& roomJson : j["rooms"]) {
        Room* room = deserializeRoom(roomJson, idMap);
        level->addRoom(room);
        if (j["current_room_name"].get<std::string>() == room->name) {
            level->currentRoom = room;
        }

    }

    for (const auto& item : j["connected_rooms"].items()) {
        std::string roomName = item.value();
        unsigned int oldId = std::stoul(item.key());

        for (Room* room : level->rooms) {
            if (room->name == roomName && idMap.find(oldId) != idMap.end()) {
                level->connectedRooms[int(idMap[oldId])] = room;
                break;
            }
        }
    }

    for (const auto& item : j["spawn_positions"].items()) {
        unsigned int oldId = std::stoul(item.key());
        if (idMap.find(oldId) != idMap.end()) {
            level->spawnPositions[int(idMap[oldId])] = { item.value()[0], item.value()[1] };
        }
    }
    
    for (const auto& item : j["spawn_directions"].items()) {
        unsigned int oldId = std::stoul(item.key());
        if (idMap.find(oldId) != idMap.end()) {
            level->spawnDirections[int(idMap[oldId])] = static_cast<TEXTURE_ASSET_ID>(item.value());
        }
    }

    return level;

}

RenderRequest RegistrySerializer::deserializeRenderRequest(const json& rrJson) {
    // printf("Loading RenderRequest component for entity %u\n", entity);
    RenderRequest rr;
    rr.used_texture = static_cast<TEXTURE_ASSET_ID>(rrJson["used_texture"]);
    rr.used_effect = static_cast<EFFECT_ASSET_ID>(rrJson["used_effect"]);
    rr.used_geometry = static_cast<GEOMETRY_BUFFER_ID>(rrJson["used_geometry"]);
    rr.hasAnimation = rrJson["hasAnimation"];

    if (rrJson.contains("animation")) {
        rr.animation.frameCount = rrJson["animation"]["frameCount"];
        rr.animation.currentFrame = rrJson["animation"]["currentFrame"];
        rr.animation.frameTime = rrJson["animation"]["frameTime"];
        rr.animation.elapsedTime = rrJson["animation"]["elapsedTime"];
        rr.animation.columns = rrJson["animation"]["columns"];
        rr.animation.rows = rrJson["animation"]["rows"];
        rr.animation.startRow = rrJson["animation"]["startRow"];
        rr.animation.startCol = rrJson["animation"]["startCol"];
        rr.animation.current_state = static_cast<AnimationState>(rrJson["animation"]["current_state"]);
    }

    rr.alpha = rrJson["alpha"];

    return rr;
    // VINCENT'S CODE EXTRACTED FUNCTION
    // updateRoomRenderRequests(game, entity_json["id"], rr);
    // if(!updateRoomRenderRequests(game, entity_json["id"], rr)) {
    //     registry.renderRequests.insert(entity, rr);
    // }
}

json RegistrySerializer::serializeLevelSystem(const LevelSystem* levelSystem) {
    json j;
    j["levels"] = json::array();

    for (const Level* level : levelSystem->levels) {
        j["levels"].push_back(serializeLevel(level));
    }

    j["current_level_name"] = levelSystem->currentLevel->name;

    return j;
}

json RegistrySerializer::serializeNPC(const NPC& npc)
{
    std::vector<std::string> dialogue_vector;
    std::queue<std::string> temp_queue = npc.dialogue;
    while (!temp_queue.empty()) {
        dialogue_vector.push_back(temp_queue.front());
        temp_queue.pop();
    }

    return {{"name", npc.name},
        {"encounter_texture_id", npc.encounter_texture_id},
        {"isInteractable", npc.isInteractable},
        {"start_encounter", npc.start_encounter},
        {"blocking_door", npc.blocking_door},
        {"blocked_door", npc.blocked_door},
        {"hasDialogue", npc.hasDialogue},
        {"dialogue", dialogue_vector},
        {"to_remove", npc.to_remove},
        {"interactIconId", npc.interactIcon.getId()},
        {"isDefeated", npc.isDefeated},
        {"isTutorialNPC", npc.isTutorialNPC},
        {"interactDistance", npc.interactDistance},
        {"dropKey", npc.dropKey}
        };
}

json RegistrySerializer::serializeDoor(const Door& door)
{
    return {{"is_open", door.is_open},
            {"required_keys", door.required_keys}};
}

json RegistrySerializer::serializeTime(const Time &time)
{
    return {{"time", time.time}};
}

Time RegistrySerializer::deserializeTime(const json& tJson)
{
    Time t;
    t.time = static_cast<float>(tJson["time"]);
    return t;
}

LevelSystem* RegistrySerializer::deserializeLevelSystem(const json& j, std::unordered_map<unsigned int, Entity>& idMap) {
    LevelSystem* levelSystem = new LevelSystem();

    for (const auto& levelJson : j["levels"]) {
        Level* level = deserializeLevel(levelJson, idMap);
        levelSystem->addLevel(level);
    }

    std::string currentLevelName = j["current_level_name"];
    for (Level* level : levelSystem->levels) {
        if (level->name == currentLevelName) {
            levelSystem->currentLevel = level;
            break;
        }
    }

    return levelSystem;
}

json RegistrySerializer::serializeKey(const Key& key){
        return {{"fade_timer", key.fadeTimer}};
}

Key RegistrySerializer::deserializeKey(const json& key_json){
    Key key;
    key.fadeTimer = key_json["fade_timer"]; 
    return key;
}

json RegistrySerializer::serializeKeyInventory(const KeyInventory& keyInventory) {
    json keyInventoryJson;
    for (const auto& key : keyInventory.keys) {
        keyInventoryJson["keys"].push_back(key.getId());
    }
    return keyInventoryJson;
}

KeyInventory RegistrySerializer::deserializeKeyInventory(const json& keyInventoryJson) {
    KeyInventory keyInventory;
    if (keyInventoryJson.contains("keys") && keyInventoryJson["keys"].is_array()) {
        for (const auto& keyId : keyInventoryJson["keys"]) {
            unsigned int oldID = keyId;
            if (idMap.find(oldID) != idMap.end()) {
                keyInventory.keys.push_back(idMap[oldID]);
            }
        }
    }
    return keyInventory;
}

json RegistrySerializer::serializePlayer(const Player& player) {
    return {
                {"lives", player.lives},
                {"lost_life", player.lost_life},
                {"lost_life_timer_ms", player.lost_life_timer_ms},
        };
}

Player RegistrySerializer::deserializePlayer(const json& player_json) {
    Player player;
    player.lives = player_json["lives"];
    player.lost_life = player_json["lost_life"];
    player.lost_life_timer_ms = player_json["lost_life_timer_ms"];
    return player;
}