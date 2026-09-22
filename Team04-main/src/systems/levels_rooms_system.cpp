#include "levels_rooms_system.hpp"
#include "world/world_init.hpp"
#include "world/world_system.hpp"

Room::Room() {};

Room::Room(const std::string& roomName) {
    name = roomName;
};

void Room::addRenderedEntity(Entity& entity) {
    rendered_entities.push_back(entity);
}

void Room::addNonRenderedEntity(Entity& entity) {
    non_rendered_entities.push_back(entity);
}

bool Room::isEntityInRoom(Entity& entity) {
    return std::find(rendered_entities.begin(), rendered_entities.end(), entity) != rendered_entities.end() ||
           std::find(non_rendered_entities.begin(), non_rendered_entities.end(), entity) != non_rendered_entities.end();
}

void Room::cleanup() {
    // printf("Cleaning room: %s\n", name.c_str());
    // printf("Rendered entities before cleanup: %zu\n", rendered_entities.size());
    // printf("Non-rendered entities before cleanup: %zu\n", non_rendered_entities.size());
    // printf("Entity render requests before cleanup: %zu\n", entity_render_requests.size());
    rendered_entities.clear();
    non_rendered_entities.clear();
    entity_render_requests.clear();
    // printf("Rendered entities after cleanup: %zu\n", rendered_entities.size());
    // printf("Non-rendered entities after cleanup: %zu\n", non_rendered_entities.size());
    // printf("Entity render requests after cleanup: %zu\n", entity_render_requests.size());
}

Level::Level(const std::string& levelName) {
    name = levelName;
    currentRoom = nullptr;
}

Level::~Level() {
    // If `rooms` are dynamically allocated, ensure to free them.
    for (Room* room : rooms) {
        delete room;
        room = nullptr;
    }
}

void Level::addRoom(Room* room) {
    rooms.push_back(room);
}

LevelSystem::LevelSystem() {
    currentLevel = nullptr;
}

void LevelSystem::loadLevel(const std::string& levelName, Entity player_character) {
    for (Level* level : levels) {
        if (level->name == levelName) {
            currentLevel = level;
            currentLevel->currentRoom = level->rooms[0];
            renderCurrentRoom(player_character);
            return;
        }
    }
}

void LevelSystem::loadLevelWithRoom(const std::string& levelName, Entity player_character, Room* room) {
    for (Level* level : levels) {
        if (level->name == levelName) {
            currentLevel = level;
            currentLevel->currentRoom = room;
            renderCurrentRoom(player_character);
            return;
        }
    }
}

void LevelSystem::addLevel(Level* level) {
    levels.push_back(level);
}

void LevelSystem::cleanUpCurrentRoom() {
    registry.renderRequests.clear();
    // printf("Registry render requests after cleanup: %zu\n", registry.renderRequests.size());
}

void LevelSystem::renderCurrentRoom(Entity player_character) {
    Room* room = currentLevel->currentRoom;
    // printf("Rendering room: %s\n", room->name.c_str());
    Motion& player_motion = registry.motions.get(player_character);
    player_motion.position = room->player_position;
    player_motion.scale = room->player_scale;
    player_motion.velocity = {0, 0};

    // printf("Rendered entities: %zu\n", room->rendered_entities.size());
    // printf("Registry render requests: %zu\n", registry.renderRequests.size());
    for (Entity entity : room->rendered_entities) {
        int id = entity;
        auto renderRequestIt = room->entity_render_requests.find(id);
        // std::cout << id << std::endl;
        if(renderRequestIt != room->entity_render_requests.end()){
            RenderRequest rr = renderRequestIt->second;
            if(!registry.renderRequests.has(entity)){
                registry.renderRequests.insert(entity, rr);
            }
        }
    }
}

void LevelSystem::reset() {
    for (Level* level : levels) {
        delete level;
    }
    levels.clear();
    currentLevel = nullptr;
}

void LevelSystem::reloadCurrentLevel(Entity player_character) {
    if (!currentLevel) return;

    currentLevel->rooms[0]->player_position = currentLevel->originalSpawnPositions[0];
    currentLevel->rooms[1]->player_position = currentLevel->originalSpawnPositions[1];
    currentLevel->rooms[2]->player_position = currentLevel->originalSpawnPositions[2];
    currentLevel->currentRoom = currentLevel->rooms[0];
    cleanUpCurrentRoom();
    renderCurrentRoom(player_character);
}

/*
#include "levels_rooms_system.hpp"
#include "world/world_init.hpp"
#include "world/world_system.hpp"

Room::Room() {};

Room::Room(const std::string& roomName) {
    name = roomName;
};

void Room::addRenderedEntity(Entity& entity) {
    rendered_entities.push_back(entity);
}

void Room::addNonRenderedEntity(Entity& entity) {
    non_rendered_entities.push_back(entity);
}

bool Room::isEntityInRoom(Entity& entity) {
    return std::find(rendered_entities.begin(), rendered_entities.end(), entity) != rendered_entities.end() ||
        std::find(non_rendered_entities.begin(), non_rendered_entities.end(), entity) != non_rendered_entities.end();
}

void Room::cleanup() {
    printf("Cleaning room: %s\n", name.c_str());
    printf("Rendered entities before cleanup: %zu\n", rendered_entities.size());
    printf("Non-rendered entities before cleanup: %zu\n", non_rendered_entities.size());
    printf("Entity render requests before cleanup: %zu\n", entity_render_requests.size());
    rendered_entities.clear();
    non_rendered_entities.clear();
    entity_render_requests.clear();
    printf("Rendered entities after cleanup: %zu\n", rendered_entities.size());
    printf("Non-rendered entities after cleanup: %zu\n", non_rendered_entities.size());
    printf("Entity render requests after cleanup: %zu\n", entity_render_requests.size());
}

Level::Level(const std::string& levelName) {
    name = levelName;
    currentRoom = nullptr;
}

void Level::addRoom(Room* room) {
    rooms.push_back(room);
}

LevelSystem::LevelSystem() {
    currentLevel = nullptr;
}

void LevelSystem::loadLevel(const std::string& levelName, Entity player_character) {
    //TODO: clean up original level
    for(Level* level : levels) {
        if(level->name == levelName) {
            // std::cout << "Original level pointer: " << &level << std::endl;
            currentLevel = level;
            //Assuming no data to read for which room to be in
            currentLevel->currentRoom = level->rooms[0];
            //currentLevel->currentRoom = level->rooms[1];
            //currentLevel->currentRoom = level->rooms[2];
            renderCurrentRoom(player_character);
            return;
        }
    }
    // std::cerr << "Level " << levelName << " not found!" << std::endl;
}

void LevelSystem::loadLevelWithRoom(const std::string& levelName, Entity player_character, Room* room) {
    //TODO: clean up original level
    for(Level* level : levels) {
        if(level->name == levelName) {
            // std::cout << "Original level pointer: " << &level << std::endl;
            currentLevel = level;
            //Assuming no data to read for which room to be in
            currentLevel->currentRoom = room;
            //currentLevel->currentRoom = level->rooms[1];
            //currentLevel->currentRoom = level->rooms[2];
            renderCurrentRoom(player_character);
            return;
        }
    }
    // std::cerr << "Level " << levelName << " not found!" << std::endl;
}

void LevelSystem::addLevel(Level* level) {
    levels.push_back(level);
}

void LevelSystem::cleanUpCurrentRoom() {
    registry.renderRequests.clear();
    // registry.patrols.clear();
    // registry.setMotions.clear();
    // currentLevel->currentRoom->non_rendered_entities.clear();
    // currentLevel->currentRoom->entity_render_requests.clear();
    printf("Registry render requests after cleanup: %zu\n", registry.renderRequests.size());
}

void LevelSystem::renderCurrentRoom(Entity player_character) {
    Room* room = currentLevel->currentRoom;
    printf("Rendering room: %s\n", room->name.c_str());
    Motion& player_motion = registry.motions.get(player_character);
    player_motion.position = room->player_position;
    // std::cout << "player spawn position: " << player_motion.position.x << "," << player_motion.position.y << std::endl;
    player_motion.scale = room->player_scale;
    player_motion.velocity = {0, 0};

    printf("rendered entities: %zu\n", room->rendered_entities.size());
    printf("registry render requests: %zu\n", registry.renderRequests.size());
    for(Entity entity: room->rendered_entities) {
        int id = entity;
        auto renderRequestIt =  room->entity_render_requests.find(id);
        RenderRequest rr= renderRequestIt->second;
        // std::cout << id << std::endl;
        registry.renderRequests.insert(entity, rr);
    }

}
*/