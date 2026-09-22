#pragma once

#include "core/ecs_registry.hpp"
#include <glm/glm.hpp>

class CameraSystem {
public:
	CameraSystem();

	void update(Entity entity, const vec2& mapSize, const vec2& windowSize);

	glm::vec2 getCameraPosition() const;

	mat3 getCameraTransform();

private:
	glm::vec2 cameraPosition;

};