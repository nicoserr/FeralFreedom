#pragma once

#include "core/ecs_registry.hpp"
#include "core/ecs.hpp"
#include "../common.hpp"
#include "systems/levels_rooms_system.hpp"


class CollisionSystem {
    public:
        CollisionSystem();
        ~CollisionSystem();

        void step(LevelSystem* levelManager);

    private:
        // CONSTS
        const float MAX_DETECTION_DISTANCE_SQUARED = 231.0f * 231.0f;

        // MAIN COLLIDING FUNCTION
        static bool collides(Entity a, Entity b);

        // GENERAL HANDLERS
        void handlePlayerCollision(Entity player, Entity non_player, LevelSystem* ls);
        void handleCollision(Entity creature, Entity collidee, LevelSystem* levelManager);

        // PLAYER COLLISIONS
        static void handlePlayerDoor(Entity player, Entity door, LevelSystem* ls);
        static void handlePlayerPatrol(Entity player, Entity patrol);
        static void handlePlayerCreature(Entity player, Entity creature);
        static void handlePlayerItem(Entity collider, Entity collidee, LevelSystem* ls);

        // NON-PLAYER COLLISONS
        static void handleCreatureObstacle(Entity collider, Entity collidee);
        static void handleCreatureCreature(Entity collider, Entity collidee);
        void handleFrictionCollision(Entity collider, Entity collidee);

        // HELPERS
        void detectAABB(LevelSystem* levelManager);
        static mat3 createTransformMatrix(const Motion &motion);
        static std::vector<vec2> calculateOBBVertices(const Motion& motion, BoundingBox& bbox);

        // CONE DETECTION
        void detectCone();
        bool playerInCone(vec2, vec2, vec2, vec2, vec2);

        // DEFUNCT
        // void handleCollision(Entity collider, Entity collidee);
        // void handleDoorCollision(Entity collider, Entity collidee, LevelSystem* levelManager);
        // void beginChase(Entity patrol_entity, vec2 player_pos);
};