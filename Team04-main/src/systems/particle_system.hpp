#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "core/ecs.hpp"
#include "core/ecs_registry.hpp"
#include "../common.hpp"
#include "render_system.hpp"
#include "levels_rooms_system.hpp"

class ParticleSystem {
    Room* currentRoom = nullptr;
public:
    struct Particle {
        glm::vec2 position, velocity;
        glm::vec4 color;
        float life;
        float size = 50;
        float maxLife;
    };

    ParticleSystem(RenderSystem& renderSystem, TEXTURE_ASSET_ID texture, int amount, float lifetime);
    void emit(vec2 position, vec2 velocity, vec4 color);
    void update(float elapsed_ms);
    void draw(const mat3& projection, const mat3& cameraTransform, Room* room);
    void initialize();
    std::vector<Particle> particles;
    void setSpawnRate(float rate);
    void setInitialParameters(vec2 position, vec2 velocity, vec4 color);
    void setCurrentRoom(Room* room) { currentRoom = room; }

    
private:
    int maxParticles;
    float life;
    TEXTURE_ASSET_ID texture;
    std::array<GLuint, texture_count> texture_gl_handles;

    GLuint particleVAO, particleVBO, instanceVBO;
    GLuint shaderProgram;

    float timeSinceLast = 0;
    int spawnRate = 350;
    int particleCount = 0;

    glm::vec2 initialPosition;
    glm::vec2 initialVelocity;
    glm::vec4 initialColor;


};