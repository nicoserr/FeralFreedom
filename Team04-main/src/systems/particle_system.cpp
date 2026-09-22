#include "particle_system.hpp"
#include <fstream> 
#include <sstream> 
#include "../common.hpp"
#include "render_system.hpp"

  

ParticleSystem::ParticleSystem(RenderSystem& renderSystem, TEXTURE_ASSET_ID texture, int maxParticles, float lifetime)
    : maxParticles(maxParticles), texture(texture),life(lifetime), texture_gl_handles(renderSystem.getTextureHandles()){
    particles.reserve(maxParticles);
    initialize();
}

void ParticleSystem::initialize() {

    if (!loadEffectFromFile(shader_path("particle.vs.glsl"), shader_path("particle.fs.glsl"), shaderProgram)) {
        fprintf(stderr, "Failed to load particle shader program\n");
        assert(false);
    }
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);

    float point[] = { 0.0f, 0.0f };

    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(point), point, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, position));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, color));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, size));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);



}

void ParticleSystem::emit(glm::vec2 position, glm::vec2 velocity, glm::vec4 color) {
    if (particleCount < maxParticles) {
        particleCount++;
        Particle particle;
        float posOffset = (rand() % 20 - 10) / 100.f + 1.f;
        float vecOffset = (rand() % 100 - 50) / 100.f + 1.f;
        particle.position = position * posOffset;
        particle.velocity = velocity * vecOffset;
        particle.life = life;
        particle.maxLife = life;
        particle.color = color;
        particle.color.a = 0;
        particles.push_back(particle);
    } 
}


void ParticleSystem::update(float elapsed_ms) {
    for (auto particle = particles.begin(); particle != particles.end();) {
        if (particle->life > 0) {
            particle->life -= elapsed_ms;
            particle->position += particle->velocity * (elapsed_ms / 1000.f);
            float lifeRatio = particle->life / particle->maxLife;

            if(lifeRatio > 0.5) {
                particle->color.a =  (1.f - lifeRatio) * 2;
            } else {
                particle->color.a = lifeRatio * 2;
            }
            ++particle;
        } else {
            particle = particles.erase(particle);
        }
    }
    timeSinceLast += elapsed_ms;
    float randomSpawn = ((rand() % 40 - 20) / 100.f + 1.f) * spawnRate;
    if (timeSinceLast >= randomSpawn && particleCount < maxParticles && particleCount > 0) {
        timeSinceLast = 0;
        emit(initialPosition, initialVelocity, initialColor);
    } else if (particleCount == maxParticles) {
        particleCount = 0;
    }

}


void ParticleSystem::draw(const mat3& projection, const mat3& cameraTransform, Room* room) {

    if(currentRoom != room){
        return;
    }

    glUseProgram(shaderProgram);
    glEnable(GL_PROGRAM_POINT_SIZE);

    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix3fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);

    GLuint cameraLoc = glGetUniformLocation(shaderProgram, "cameraTransform");
    glUniformMatrix3fv(cameraLoc, 1, GL_FALSE, &cameraTransform[0][0]);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_gl_handles[static_cast<size_t>(texture)]);
    GLuint textureLoc = glGetUniformLocation(shaderProgram, "particleTexture");
    glUniform1i(textureLoc, 0);
    glBindVertexArray(particleVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(Particle), particles.data());


    mat3 transform = glm::mat3(1.0f);
    GLuint transformLoc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix3fv(transformLoc, 1, GL_FALSE, &transform[0][0]);

    glDrawArraysInstanced(GL_POINTS, 0, 1, particles.size());

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

}

void ParticleSystem::setSpawnRate(float rate) {
    spawnRate = rate;
}

void ParticleSystem::setInitialParameters(vec2 position, vec2 velocity, vec4 color){
    initialPosition = position;
    initialVelocity = velocity;
    initialColor = color;
}