#define GL3W_IMPLEMENTATION
#include <gl3w.h>

#include <chrono>
#include <iostream>

#include "systems/render_system.hpp"
#include "world/world_system.hpp"
#include "core/ecs.hpp"
#include "serialization/registry_serializer.hpp"
#include "systems/physics_system.hpp"
#include "systems/collision_system.hpp"
#include "systems/pathfinding_system.hpp"
#include "systems/levels_rooms_system.hpp"
#include "systems/text_renderer.hpp"
#include "systems/shadow_renderer.hpp"

using Clock = std::chrono::high_resolution_clock;

int main() {
    WorldSystem world;
    RenderSystem renderer;
    TextRenderer text_renderer;
    ShadowRenderer shadow_renderer;
	PhysicsSystem physics;
    CollisionSystem collision_manager;
	PathFindingSystem pathfinding;
    LevelSystem level_manager;

    // Initialize window
    GLFWwindow* window = world.create_window();
    	if (!window) {
		// Time to read the error message
		printf("Press any key to exit");
		getchar();
		return EXIT_FAILURE;
	}

    renderer.init(window);
    world.init(&renderer, &level_manager, &collision_manager, &text_renderer, &shadow_renderer, &pathfinding);


    std::string font_filename = PROJECT_SOURCE_DIR + std::string("data/fonts/dogicapixel.ttf");
	unsigned int font_default_size = 48;

    text_renderer.init(font_filename, font_default_size);
    shadow_renderer.init(renderer.getWindow());
    renderer.setShadowMap(shadow_renderer.shadowMap);
    shadow_renderer.clearShadows();

    auto t = Clock::now();
    while(!world.is_over()) {
        glfwPollEvents();

        auto now = Clock::now();
        float elapsed_ms =
            (float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
		t = now;

        world.step(elapsed_ms);
        physics.step(elapsed_ms);
        // renderer.draw(); moved inside of step

        // pathfinding.step(); moved inside of world.step
    }

	// world.cleanup(); // Re-added, deletes saved jsons. Uncomment if you want to keep them.

    return EXIT_SUCCESS;
}