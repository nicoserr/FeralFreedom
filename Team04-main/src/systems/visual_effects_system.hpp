#pragma once
#include "core/ecs.hpp"
#include "core/components.hpp"

class VisualEffectsSystem {
public:
    void addEffect(Entity entity, const std::string& effectType, float duration, float amount);
    void update(float elapsed_ms, GLuint shaderProgram);
    void applyEffect(Entity entity, const VisualEffect& effect, GLuint shaderProgram) const;

private:

};
