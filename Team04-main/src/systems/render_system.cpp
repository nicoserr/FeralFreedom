#include "render_system.hpp"
#include "core/ecs_registry.hpp"
#include <SDL.h>

RenderSystem::RenderSystem() : cameraTransform(mat3(1.0f)) {}

void RenderSystem::setAnimation(RenderRequest& renderRequest, AnimationState state, std::unordered_map<AnimationState, Animation> animationMap) {
	if (renderRequest.animation.current_state != state) {
		renderRequest.animation.current_state = state;
		const Animation& params = animationMap[state];
		Animation& animation = renderRequest.animation;
		animation.frameCount = params.frameCount;
		animation.currentFrame = 0;
		animation.frameTime = params.frameTime;
		animation.elapsedTime = 0.0f;
		animation.columns = params.columns;
		animation.rows = params.rows;
		animation.startRow = params.startRow;
		animation.startCol = params.startCol;
	}
}


void RenderSystem::drawTexturedMesh(Entity entity, const mat3 &projection) {
	Motion &motion = registry.motions.get(entity);
    Transform transform;
    transform.translate(motion.position);

    //the following lines will make it so that any ui element
    //have their motion.angle actually affect their visual rotation
    //notably, light cones of patrols use this feature to stay 
    //properly oriented
    if (registry.uiElements.has(entity) || registry.rotatables.has(entity)) {
        transform.rotate(motion.angle);
    }

    transform.scale(motion.scale);

    assert(registry.renderRequests.has(entity));
    const RenderRequest &render_request = registry.renderRequests.get(entity);

    const GLuint used_effect_enum = (GLuint)render_request.used_effect;
    assert(used_effect_enum != (GLuint)EFFECT_ASSET_ID::EFFECT_COUNT);
    const GLuint program = (GLuint)effects[used_effect_enum];

	// Bind default VAO
	glBindVertexArray(vao);
    glUseProgram(program);
    current_shader = program;
    gl_has_errors();

    assert(render_request.used_geometry != GEOMETRY_BUFFER_ID::GEOMETRY_COUNT);
    const GLuint vbo = vertex_buffers[(GLuint)render_request.used_geometry];
    const GLuint ibo = index_buffers[(GLuint)render_request.used_geometry];

    if(render_request.used_effect == EFFECT_ASSET_ID::TEXTURED) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        gl_has_errors();
        GLint in_position_loc = glGetAttribLocation(program, "in_position");
        GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");

        gl_has_errors();
        assert(in_texcoord_loc >= 0);

        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void *)0);

        gl_has_errors();

        glEnableVertexAttribArray(in_texcoord_loc);
        glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));

        // Enabling and binding texture to slot 0
		glActiveTexture(GL_TEXTURE0);
		gl_has_errors();

        assert(registry.renderRequests.has(entity));
		GLuint texture_id =
			texture_gl_handles[(GLuint)registry.renderRequests.get(entity).used_texture];

		glBindTexture(GL_TEXTURE_2D, texture_id);
		gl_has_errors();

    	// Set default texture coordinates for non-animated meshes
    	glUniform2f(glGetUniformLocation(program, "uTexCoordMin"), 0.0f, 0.0f);
    	glUniform2f(glGetUniformLocation(program, "uTexCoordMax"), 1.0f, 1.0f);

        // Getting uniform locations for glUniform* calls
        GLint color_uloc = glGetUniformLocation(program, "fcolor");
        const vec3 color = vec3(1);
        glUniform3fv(color_uloc, 1, (float *)&color);
        GLint alpha_loc = glGetUniformLocation(program, "alpha");
        glUniform1f(alpha_loc, render_request.alpha);
        gl_has_errors();

        // Get number of indices from index buffer, which has elements uint16_t
        GLint size = 0;
        glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
        gl_has_errors();

        GLsizei num_indices = size / sizeof(uint16_t);
        // GLsizei num_triangles = num_indices / 3;

        GLint currProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
        // Setting uniform values to the currently bound program
        GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
        glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float *)&transform.mat);
        GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
        glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float *)&projection);
        GLuint camera_loc = glGetUniformLocation(currProgram, "cameraTransform");
        glUniformMatrix3fv(camera_loc, 1, GL_FALSE, (float*)&cameraTransform);
        gl_has_errors();
        // Drawing of num_indices/3 triangles specified in the index buffer
        glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
        gl_has_errors();
    } else if (render_request.used_effect == EFFECT_ASSET_ID::BACKPACK)
    {
        glBindVertexArray(backpack_vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        gl_has_errors();
        transform.rotate(motion.angle);
        // GLint in_color_loc = glGetAttribLocation(program, "in_color");;
        // std::cout << in_color_loc << std::endl;
        GLint in_position_loc = glGetAttribLocation(program, "in_position");
        gl_has_errors();

        glEnableVertexAttribArray(in_position_loc);
        glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
                            sizeof(ColoredVertex), (void *)0);
        gl_has_errors();

        // glEnableVertexAttribArray(in_color_loc);
        // glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE,
        //                     sizeof(ColoredVertex), (void *)sizeof(vec3));
        // gl_has_errors();
    }

// Getting uniform locations for glUniform* calls
    GLint color_uloc = glGetUniformLocation(program, "fcolor");
    const vec3 color = vec3(1);
    glUniform3fv(color_uloc, 1, (float *)&color);
    GLint alpha_loc = glGetUniformLocation(program, "alpha");
    glUniform1f(alpha_loc, render_request.alpha);
    gl_has_errors();

    // Get number of indices from index buffer, which has elements uint16_t
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    gl_has_errors();

    GLsizei num_indices = size / sizeof(uint16_t);
    // GLsizei num_triangles = num_indices / 3;

    GLint currProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
    // Setting uniform values to the currently bound program
    GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
    glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float *)&transform.mat);
    GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
    glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float *)&projection);
    GLuint camera_loc = glGetUniformLocation(currProgram, "cameraTransform");
    glUniformMatrix3fv(camera_loc, 1, GL_FALSE, (float*)&cameraTransform);
    gl_has_errors();
    // Drawing of num_indices/3 triangles specified in the index buffer
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    gl_has_errors();
}

void RenderSystem::setShadowMap(GLuint shadowMap_arg)
{
    shadowMap = shadowMap_arg;
}

void RenderSystem::drawAnimatedMesh(Entity entity, const mat3 &projection, float elapsed_ms_since_last_update)
{
    Transform transform;
    Motion &motion = registry.motions.get(entity);
    transform.translate(motion.position);
    transform.scale(motion.scale);

    // Access the RenderRequest and Animation data
    assert(registry.renderRequests.has(entity));
    RenderRequest &render_request = registry.renderRequests.get(entity);
    Animation &animation = render_request.animation;

	// Only update the frame if frameTime > 0
	if (animation.frameTime > 0.0f) {
		animation.elapsedTime += elapsed_ms_since_last_update;
		if (animation.elapsedTime >= animation.frameTime) {
			animation.currentFrame = (animation.currentFrame + 1) % animation.frameCount;
			animation.elapsedTime = 0.0f;
		}
	}

	// Calculate frame dimensions in texture coordinates
	float frameWidth = 1.0f / animation.columns;
	float frameHeight = 1.0f / animation.rows;

	// Calculate the column and row for the current frame
	int frameCol = (animation.startCol + animation.currentFrame) % animation.columns;
	int frameRow = animation.startRow + (animation.startCol + animation.currentFrame) / animation.columns;

	// Calculate texture coordinates
	float uMin = frameCol * frameWidth;
	float vMin = frameRow * frameHeight;
	float uMax = uMin + frameWidth;
	float vMax = vMin + frameHeight;

	// Apply horizontal flip if we don't have a texture for that
	if (render_request.flip_horizontal) {
		std::swap(uMin, uMax);
	}

    // Bind the shader program
    const GLuint program = (GLuint)effects[(GLuint)render_request.used_effect];
    glUseProgram(program);
    current_shader = program;
    gl_has_errors();

    // Bind geometry
    const GLuint vbo = vertex_buffers[(GLuint)render_request.used_geometry];
    const GLuint ibo = index_buffers[(GLuint)render_request.used_geometry];
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    gl_has_errors();

    // Set vertex attributes
    GLint in_position_loc = glGetAttribLocation(program, "in_position");
    GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
    assert(in_texcoord_loc >= 0);

    // Set the position attribute
    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void *)0);
    gl_has_errors();

    // Set the texture coordinate attribute
    glEnableVertexAttribArray(in_texcoord_loc);
    glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));

    // Pass the calculated texture coordinates to the shader
    glUniform2f(glGetUniformLocation(program, "uTexCoordMin"), uMin, vMin);
    glUniform2f(glGetUniformLocation(program, "uTexCoordMax"), uMax, vMax);

    // Enable and bind texture to slot 0
    glActiveTexture(GL_TEXTURE0);
    GLuint texture_id = texture_gl_handles[(GLuint)render_request.used_texture];
    glBindTexture(GL_TEXTURE_2D, texture_id);
    gl_has_errors();

    GLint alpha_loc = glGetUniformLocation(program, "alpha");
    glUniform1f(alpha_loc, render_request.alpha);

    // Set uniforms
    GLuint transform_loc = glGetUniformLocation(program, "transform");
    glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float *)&transform.mat);
    GLuint projection_loc = glGetUniformLocation(program, "projection");
    glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float *)&projection);
    GLuint camera_loc = glGetUniformLocation(program, "cameraTransform");
    glUniformMatrix3fv(camera_loc, 1, GL_FALSE, (float*)&cameraTransform);
    gl_has_errors();

    // Draw the mesh
    GLint size = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
    GLsizei num_indices = size / sizeof(uint16_t);
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
    gl_has_errors();
}

void RenderSystem::drawToScreen(bool is_paused, vec4 overlay_color, float light_amount) {
    // Setting shaders
    // Use the textured shader
    glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::BLACK]);
    current_shader = effects[(GLuint)EFFECT_ASSET_ID::BLACK];
    gl_has_errors();

    // Clearing backbuffer
    int w, h;
    glfwGetFramebufferSize(window, &w, &h); // Get the framebuffer size
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);
    glDepthRange(0, 10);
    glClearColor(1.f, 0, 0, 1.0); // Clear color
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_has_errors();

    // Enabling alpha channel for textures
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    // Draw the screen texture on the quad geometry
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
    gl_has_errors();
    
    const GLuint program = effects[(GLuint)EFFECT_ASSET_ID::BLACK];
	// Set clock
	GLuint time_uloc = glGetUniformLocation(program, "time");
	glUniform1f(time_uloc, (float)(glfwGetTime() * 10.0f));

	// Pass in the redness factor to the shader
	GLuint redness_loc = glGetUniformLocation(program, "redness_timer");
	float redness_timer = -1.f;
	for (Entity e : registry.visualEffects.entities) {
		const VisualEffect& effect = registry.visualEffects.get(e);
		if (effect.type == "redness") {
			redness_timer = effect.amount;
			break; // add only one redness
		}
	}
	glUniform1f(redness_loc, redness_timer);
	gl_has_errors();

    GLuint overlay_loc = glGetUniformLocation(program, "overlay_color");
    glUniform4f(overlay_loc, overlay_color.r, overlay_color.g, overlay_color.b, overlay_color.a);
    gl_has_errors();

    GLuint light_loc = glGetUniformLocation(program, "vignette_amount");
    glUniform1f(light_loc, light_amount);
    gl_has_errors();

	// Pass the pause state to the shader
	GLuint is_paused_loc = glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::BLACK], "is_paused");
	glUniform1i(is_paused_loc, is_paused ? 1 : 0);

    // Set the vertex position and vertex texture coordinates (both stored in the same VBO)
    GLint in_position_loc = glGetAttribLocation(effects[(GLuint)EFFECT_ASSET_ID::BLACK], "in_position");
    glEnableVertexAttribArray(in_position_loc);
    glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
    gl_has_errors();

    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    gl_has_errors();

    glUniform1i(glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::BLACK], "screen_texture"), 0);
    glUniform1i(glGetUniformLocation(effects[(GLuint)EFFECT_ASSET_ID::BLACK], "shadow_map"), 1);

	// Draw
	glDrawElements(
		GL_TRIANGLES, 3, GL_UNSIGNED_SHORT,
		nullptr); // one triangle = 3 vertices; nullptr indicates that there is
				  // no offset from the bound index buffer
	gl_has_errors();
}

void RenderSystem::draw(float elapsed_ms_since_last_update, bool is_paused, vec4 overlay_color, float light_amount) {
    //std::cout <<" IN DRAW" << std::endl;
    // Getting size of window
	int w, h;
	glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays
    glBindVertexArray(vao);
	// First render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();
	// Clearing backbuffer
	glViewport(0, 0, w, h);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST); // native OpenGL does not work with a depth buffer
							  // and alpha blending, one would have to sort
							  // sprites back to front
	gl_has_errors();
	mat3 projection_2D = createProjectionMatrix();
    mat3 identityMatrix = mat3(1.0f);

	// std::vector<Entity> sorted_entities = registry.renderRequests.entities;
	// std::sort(sorted_entities.begin(), sorted_entities.end(), [](Entity a, Entity b) {
	// 	return registry.motions.get(a).z < registry.motions.get(b).z;
	// });

	// Draw all textured meshes that have a position and size component
	for (Entity entity : registry.renderRequests.entities)
	{   // if ui element, skip because we will render them after draw to screen.
		if (!registry.motions.has(entity) || registry.uiElements.has(entity))
			continue;
		RenderRequest& renderRequest = registry.renderRequests.get(entity);

		if (renderRequest.hasAnimation) {
			drawAnimatedMesh(entity, projection_2D, elapsed_ms_since_last_update);
		} else {
			drawTexturedMesh(entity, projection_2D);
		}
	}

	// Truely render to the screen
	drawToScreen(is_paused, overlay_color, light_amount);

	// Sorts the UI elements by z value. If you remove this, inventory items will render below the inventory box when you change rooms.
	std::vector<Entity> sorted_ui_entities = registry.uiElements.entities;
	std::stable_sort(sorted_ui_entities.begin(), sorted_ui_entities.end(), [](Entity a, Entity b) {
		if (registry.motions.has(a) && registry.motions.has(b)) {
			return registry.motions.get(a).z < registry.motions.get(b).z;
		}
		return false;
	});

    // Assumes we haven't unbind the frame_buffer from drawToScreen()
    glEnable(GL_BLEND);
    for (Entity entity : sorted_ui_entities) {
        if (registry.renderRequests.has(entity)) {
            cameraTransform = identityMatrix;
            if (registry.renderRequests.get(entity).hasAnimation) {
                drawAnimatedMesh(entity, projection_2D, elapsed_ms_since_last_update);
            } else {
                drawTexturedMesh(entity, projection_2D);
            }
        }
    }

	gl_has_errors();
    glBindVertexArray(0);
}

mat3 RenderSystem::createProjectionMatrix()
{
	// Fake projection matrix, scales with respect to window coordinates
	float left = 0.f;
	float top = 0.f;

	float right = (float) window_width_px;
	float bottom = (float) window_height_px;

	float sx = 2.f / (right - left);
	float sy = 2.f / (top - bottom);
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);
	return {{sx, 0.f, 0.f}, {0.f, sy, 0.f}, {tx, ty, 1.f}};
}

void RenderSystem::drawOverlayBox(float center_x, float center_y, float box_width, float box_height, glm::vec4 color) const {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // clean to black screen

	GLuint program = getEffectProgram(EFFECT_ASSET_ID::BLACK);
	glUseProgram(program);

	// Enable blending for transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float norm_center_x = center_x / window_width_px;
	float norm_center_y = center_y / window_height_px;

	GLuint boxCenter_loc = glGetUniformLocation(program, "boxCenter");
	GLuint boxSize_loc = glGetUniformLocation(program, "boxSize");
	GLint boxColor_loc = glGetUniformLocation(program, "boxColor");

	glUniform2f(boxCenter_loc, norm_center_x, norm_center_y);
	glUniform2f(boxSize_loc, box_width / window_width_px, box_height / window_height_px);
	glUniform4f(boxColor_loc, color.r, color.g, color.b, color.a);
}

void RenderSystem::drawTransform(float elapsed_ms_since_last_update, const mat3& transform) {
    //std::cout << "transform matrix:" << std::endl;
    //printMat3(transform);
    cameraTransform = transform;
}

mat3 RenderSystem::createCameraProjection(const vec2& cameraPosition){
    float left = cameraPosition.x - (window_width_px / 2.0f);
    float right = cameraPosition.x + (window_width_px / 2.0f);
    float top = cameraPosition.y - (window_height_px / 2.0f);
    float bottom = cameraPosition.y + (window_height_px / 2.0f);

    float sx = 2.f / (right - left);
    float sy = 2.f / (bottom - top);
    float tx = -(right + left) / (right - left);
    float ty = -(bottom + top) / (bottom - top);

    return {{sx, 0.0f, 0.0f}, {0.0f, sy, 0.0f}, {tx, ty, 1.0f}};
}

void RenderSystem::printMat3(const mat3& matrix) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            std::cout << matrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
