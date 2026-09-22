#include "visual_effects_system.hpp"
#include "core/ecs_registry.hpp"

void VisualEffectsSystem::addEffect(Entity entity, const std::string& effectType, float duration, float amount) {
    if (!registry.visualEffects.has(entity)) {
        VisualEffect effect = { duration, amount, effectType };
        registry.visualEffects.emplace(entity, effect);
    }
}

void VisualEffectsSystem::update(float elapsed_ms, GLuint shaderProgram) {
    if (shaderProgram == 0) {
        std::cerr << "Shader program is not valid." << std::endl;
        return;
    }
    glUseProgram(shaderProgram);
    for (Entity e : registry.visualEffects.entities) {
        VisualEffect& effect = registry.visualEffects.get(e);
        effect.duration -= elapsed_ms;

        if (effect.type == "redness") {
            effect.amount += elapsed_ms;
        }

        if (effect.duration <= 0) {
            registry.visualEffects.remove(e);
        } else {
            applyEffect(e, effect, shaderProgram);
        }
    }
}

void VisualEffectsSystem::applyEffect(Entity entity, const VisualEffect& effect, GLuint shaderProgram) const{
    if (effect.type == "redness") {
        GLuint redness_timer_loc = glGetUniformLocation(shaderProgram, "redness_timer");
        if (redness_timer_loc == -1) {
            std::cerr << "Uniform 'redness_timer' not found in shader program." << std::endl;
            return;
        }
        glUniform1f(redness_timer_loc, effect.amount);
    }
}
