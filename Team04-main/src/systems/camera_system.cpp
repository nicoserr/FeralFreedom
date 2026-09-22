#include "systems/camera_system.hpp"

CameraSystem::CameraSystem() : cameraPosition(0, 0){}

void CameraSystem::update(Entity player, const vec2& mapSize, const vec2& windowSize) {
	if (registry.motions.has(player)) {
		Motion& playerMotion = registry.motions.get(player);

		vec2 halfWindow = windowSize / 2.f;

		float newCameraX = std::max(halfWindow.x, std::min(mapSize.x - halfWindow.x, playerMotion.position.x));
		float newCameraY = std::max(halfWindow.y, std::min(mapSize.y - halfWindow.y, playerMotion.position.y));

		cameraPosition = { newCameraX - halfWindow.x, newCameraY - halfWindow.y};
	}
}

vec2 CameraSystem::getCameraPosition() const {
	return cameraPosition;
}

mat3 CameraSystem::getCameraTransform() {
	mat3 transform = mat3(1);

	transform[2].x = -cameraPosition.x;
	transform[2].y = -cameraPosition.y;

	return transform;
}