#pragma once
#pragma once

#pragma once

#include "core/ecs.hpp"
#include "core/ecs_registry.hpp"
#include "../common.hpp"
#include <string>
#include <vector>
#include <map>
#include <systems/text_renderer.hpp>
#include <systems/pathfinding_system.hpp>

class Room {
public:
    std::string name;
    std::vector<Entity> rendered_entities;
    std::vector<Entity> non_rendered_entities;
    std::map<int, RenderRequest> entity_render_requests;
    vec2 player_position;
    vec2 player_scale;
    vec2 gridDimensions;
    int room_height;
    int room_width;
    std::unordered_map<Entity, Entity, EntityHash> meshToTextureMap;
    std::vector<std::vector<Node>>* a_star_grid;

    Room();
    Room(const std::string& roomName);
    operator std::string();

    void addRenderedEntity(Entity& entity);
    void addNonRenderedEntity(Entity& entity);
    void initPathfinding();
    bool isEntityInRoom(Entity& entity);
    void cleanup();
    void remove_entity_from_room(Entity entity) {
        remove_entity_from_rendered_entities(entity);
        remove_entity_from_non_rendered_entities(entity);
        remove_entity_from_render_requests((int)entity);
    }

private:
    void remove_entity_from_rendered_entities(Entity entity) {
        rendered_entities.erase(
            std::remove(rendered_entities.begin(), rendered_entities.end(), entity),
            rendered_entities.end()
        );
    }
    void remove_entity_from_non_rendered_entities(Entity entity) {
        non_rendered_entities.erase(
            std::remove(non_rendered_entities.begin(), non_rendered_entities.end(), entity),
            non_rendered_entities.end()
        );
    }
    void remove_entity_from_render_requests(int entity_id) {
        entity_render_requests.erase(entity_id);
    }
};

class Level {
public:
    std::string name;
    std::vector<Room*> rooms;
    Room* currentRoom;
    std::map<int, Room*> connectedRooms;
    std::map<int, vec2> spawnPositions;
    std::map<int, TEXTURE_ASSET_ID> spawnDirections;
    std::vector<vec2> originalSpawnPositions;

    Level(const std::string& levelName);
    ~Level();
    operator std::string();

    void addRoom(Room* room);
};

class LevelSystem {
public:
    LevelSystem();

    void loadLevel(const std::string& levelName, Entity player_character);
    void loadLevelWithRoom(const std::string& levelName, Entity player_character, Room* room);
    void reloadCurrentLevel(Entity player_character);
    void addLevel(Level* level);
    void renderCurrentRoom(Entity player_character);
    void cleanUpCurrentRoom();
    void reset();

    Entity health_bar;
    std::vector<Level*> levels;
    Level* currentLevel;
};

/*
#include "core/ecs.hpp"
#include "core/ecs_registry.hpp"
#include "../common.hpp"

#include <string>
#include <map>
#include <systems/text_renderer.hpp>


class Room {
    public:
        std::string name;
        std::vector<Entity> rendered_entities;
        std::vector<Entity> non_rendered_entities;
        std::map<int, RenderRequest> entity_render_requests;
        vec2 player_position; //Position to render player at, changes based on exit/entrance taken
        vec2 player_scale;

        Room();
        Room(const std::string& roomName);
        operator std::string() { return name; };

        void addRenderedEntity(Entity& entity);
        void addNonRenderedEntity(Entity& entity);
        vec2 gridDimensions;
        void cleanup();
        bool isEntityInRoom(Entity& entity);
};

class Level {
    public:
        std::string name;
        std::vector<Room*> rooms;
        Room* currentRoom;

        Level(const std::string& levelName);
        operator std::string() { return name; };

        void addRoom(Room* room);
        std::map<int, Room*> connectedRooms; //Map connected rooms using Bounding box entity Id
        std::map<int, vec2> spawnPositions; //Map exit entity id's with new spawn position after coming back
        std::map<int, TEXTURE_ASSET_ID> spawnDirections; //Map exit with direction player should be facing

};

class LevelSystem {
    public:
        LevelSystem();
        void loadLevel(const std::string& levelName, Entity player_character);
        void loadLevelWithRoom(const std::string& levelName, Entity player_character, Room* room);
        void addLevel(Level* level);
        void renderCurrentRoom(Entity player_character);
        void cleanUpCurrentRoom();

        // void reset();
        Entity health_bar;
        std::vector<Level*> levels;
        Level* currentLevel;
};
*/