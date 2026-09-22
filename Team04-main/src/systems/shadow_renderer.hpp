#pragma once

#include <string>
#include <gl3w.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <array>
#include <utility>

#include <common.hpp>
#include "core/components.hpp"
#include "core/ecs.hpp"
#include "core/ecs_registry.hpp"
#include "render_system.hpp"
#include "levels_rooms_system.hpp"

class ShadowRenderer {
public:
    // Initialize the text renderer
    void init(GLFWwindow* window_arg);

    // Render the specified text
    void RenderShadows(LevelSystem* ls, mat3 camera);

    void clearShadows();

    GLuint shadowMap;

    GLuint getShadowShader() {return shadowShader;}

private:
    struct segment {
        vec2 a, b;
    };

    GLuint shadowShader;
    GLuint shadowVAO, shadowVBO, shadowIBO;
    GLuint shadowFBO;
    GLuint shadowRBO;
    GLFWwindow* window;

    std::vector<vec3> vertices;
};