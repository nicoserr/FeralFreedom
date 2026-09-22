#include "shadow_renderer.hpp"

// Initialize FreeType and text rendering components
void ShadowRenderer::init(GLFWwindow* window_arg) {
    window = window_arg;

    if (!loadEffectFromFile(shader_path("shadow.vs.glsl"), shader_path("shadow.fs.glsl"), shadowShader)) {
        fprintf(stderr, "Failed to load font shader program\n");
        assert(false);
    }

    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);  

    // The texture we're going to render to
    glGenTextures(1, &shadowMap);
    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, shadowMap);

    int frame_buffer_width_px, frame_buffer_height_px;
	glfwGetFramebufferSize(window, &frame_buffer_width_px, &frame_buffer_height_px);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadowMap, 0);
    // Give an empty image to OpenGL ( the last "0" )
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, frame_buffer_width_px, frame_buffer_height_px, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    // Poor filtering. Needed !
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &shadowRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, shadowRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, frame_buffer_width_px, frame_buffer_height_px);  
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, shadowRBO);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0); 

    glGenVertexArrays(1, &shadowVAO);
    glGenBuffers(1, &shadowVBO);
    glGenBuffers(1, &shadowIBO);
    gl_has_errors();

    glBindVertexArray(shadowVAO);

    vertices = std::vector<vec3>(4);

    vertices[0] = {0.f, 0.f, 0.f};
    vertices[1] = {0.f, 0.f, 0.f};
    vertices[2] = {0.f, 0.f, 0.f};
    vertices[3] = {0.f, 0.f, 0.f};

    glBindBuffer(GL_ARRAY_BUFFER, shadowVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);
    gl_has_errors();

    std::vector<uint16_t> indices = {
        0, 1, 2, // first triangle
        2, 3, 1  // second triangle
    };

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shadowIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), indices.data(), GL_STATIC_DRAW);
    gl_has_errors();

    GLint vertex_loc = glGetAttribLocation(shadowShader, "vertex");
    glEnableVertexAttribArray(vertex_loc);
    glVertexAttribPointer(vertex_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void *)0);
    gl_has_errors();

    glBindVertexArray(0);
    gl_has_errors();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl_has_errors();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    gl_has_errors();
}


void ShadowRenderer::RenderShadows(LevelSystem* ls, mat3 camera) {
    glUseProgram(shadowShader);
    glBindVertexArray(shadowVAO);
    gl_has_errors();

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

    int frame_buffer_width_px, frame_buffer_height_px;
    glfwGetFramebufferSize(window, &frame_buffer_width_px, &frame_buffer_height_px);
    glViewport(0,0,frame_buffer_width_px, frame_buffer_height_px);
    glClearColor(1.0,1.0,1.0,1.0);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_has_errors();

    //creates a 2D projection
    mat3 projection = createShadowProjectionMatrix();
    GLuint projection_loc = glGetUniformLocation(shadowShader, "projection");
    glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float *)&projection);
    gl_has_errors();

    //camera
    GLuint camera_loc = glGetUniformLocation(shadowShader, "cameraTransform");
    glUniformMatrix3fv(camera_loc, 1, GL_FALSE, (float *)&camera);
    gl_has_errors();

    vec2 cameraCenter = {camera[2].x, camera[2].y};

    // pass in position of the player as the location of the "light source"
    GLuint light_position_loc = glGetUniformLocation(shadowShader, "light_position");
    vec2 light_position = {0.f,0.f};
    if (registry.players.size() > 0) {
        Entity player = *registry.players.entities.begin();
        light_position = registry.motions.get(player).position;
        if (cameraCenter.x == 1.0f) {  //very scuffed, if there is a bug, look here!!!!
            light_position.x -= window_width_px/2.0f;
            light_position.y -= window_height_px/2.0f;
        } else {
            light_position.x -= window_width_px/2.0f - cameraCenter.x;
            light_position.y -= window_height_px/2.0f - cameraCenter.y;
        }
    }
    

    glUniform2fv(light_position_loc, 1, (float *)&light_position);
    //std::cout << "cat position:" << light_position.x << " " << light_position.y << std::endl;
    
    //loop through every shadow caster
    std::vector<Entity> colliders_list = ls->currentLevel->currentRoom->non_rendered_entities;
    for (Entity e : colliders_list) {
        Collider& collider = registry.colliders.get(e);
        if (collider.type != COLLIDER_TYPE::OBSTACLE || collider.transparent) {
            continue;
        }

        BoundingBox& box = registry.boundingBoxes.get(e);
        Motion& motion = registry.motions.get(e);

        // potientially GPU accelerate this if I have time
        vec2 a = {motion.position.x + box.offset.x + box.width/2.f, motion.position.y + box.offset.y - box.height/2.f};
        vec2 b = {motion.position.x + box.offset.x + box.width/2.f, motion.position.y + box.offset.y + box.height/2.f};

        vertices[0] = {a.x, a.y, 0.f};
        vertices[1] = {a.x, a.y, 1.f};
        vertices[2] = {b.x, b.y, 0.f};
        vertices[3] = {b.x, b.y, 1.f};

        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, shadowVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shadowIBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices[0]) * vertices.size(), vertices.data());
        gl_has_errors();
   
        // render quad
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
        gl_has_errors();

        /////////////////////////////////////////////////////////////////

        // potientially GPU accelerate this if I have time
        a = {motion.position.x + box.offset.x - box.width/2.f, motion.position.y + box.offset.y - box.height/2.f};
        b = {motion.position.x + box.offset.x - box.width/2.f, motion.position.y + box.offset.y + box.height/2.f};

        vertices[0] = {a.x, a.y, 0.f};
        vertices[1] = {a.x, a.y, 1.f};
        vertices[2] = {b.x, b.y, 0.f};
        vertices[3] = {b.x, b.y, 1.f};

        // update content of VBO memory
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices[0]) * vertices.size(), vertices.data());

        // render quad
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

        /////////////////////////////////////////////////////////////////

        // potientially GPU accelerate this if I have time
        a = {motion.position.x + box.offset.x - box.width/2.f, motion.position.y + box.offset.y + box.height/2.f};
        b = {motion.position.x + box.offset.x + box.width/2.f, motion.position.y + box.offset.y + box.height/2.f};

        vertices[0] = {a.x, a.y, 0.f};
        vertices[1] = {a.x, a.y, 1.f};
        vertices[2] = {b.x, b.y, 0.f};
        vertices[3] = {b.x, b.y, 1.f};

        // update content of VBO memory
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices[0]) * vertices.size(), vertices.data());

        // render quad
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

        /////////////////////////////////////////////////////////////////

        // potientially GPU accelerate this if I have time
        a = {motion.position.x + box.offset.x - box.width/2.f, motion.position.y + box.offset.y - box.height/2.f};
        b = {motion.position.x + box.offset.x + box.width/2.f, motion.position.y + box.offset.y - box.height/2.f};

        vertices[0] = {a.x, a.y, 0.f};
        vertices[1] = {a.x, a.y, 1.f};
        vertices[2] = {b.x, b.y, 0.f};
        vertices[3] = {b.x, b.y, 1.f};

        // update content of VBO memory
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices[0]) * vertices.size(), vertices.data());

        // render quad
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowRenderer::clearShadows() {
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClearColor(1.0f,1.0f,1.0f,1.0f);
    glClearDepth(1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl_has_errors();
}