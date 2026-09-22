#pragma once

#include <vector>
#include <map>
#include <array>
#include <utility>

#include <common.hpp>
#include "systems/visual_effects_system.hpp"
#include "core/components.hpp"
#include "core/ecs.hpp"

class RenderSystem {

    std::array<GLuint, texture_count> texture_gl_handles;
    std::array<ivec2, texture_count> texture_dimensions;

    const std::array<std::string, texture_count> texture_paths = {
        textures_path("player/black_cat_spritesheet.png"),
        textures_path("player/black_cat_down.png"),
        textures_path("player/black_cat_right.png"),
        textures_path("player/black_cat_left.png"),
        textures_path("player/black_cat_up.png"),
        textures_path("player/black_cat_up_left.png"),
        textures_path("player/black_cat_up_right.png"),
        textures_path("player/black_cat_down_left.png"),
        textures_path("player/black_cat_down_right.png"),
        textures_path("patrol_down.png"),
        textures_path("patrol_left.png"),
        textures_path("patrol_right.png"),
        textures_path("patrol_up.png"),
        textures_path("kennel_room/Floor_top.png"),
        textures_path("kennel_room/Floor_center.png"),
        textures_path("kennel_room/top_wall.png"),
        textures_path("kennel_room/side_wall_1.png"),
        textures_path("kennel_room/side_wall_bottom.png"),
        textures_path("kennel_room/Side_wall_corner.png"),
        textures_path("kennel_room/Bottom_wall.png"),
        textures_path("kennel_room/Jail_door.png"),
        textures_path("kennel_room/Jail_wall.png"),
        textures_path("kennel_room/wall_hole2.png"),
        textures_path("kennel_room/middle_wall.png"),
        textures_path("kennel_room/cat_pillow_1.png"),
        textures_path("kennel_room/cat_pillow_2.png"),
        textures_path("kennel_room/cat_pillow_3.png"),
        textures_path("tunnel/tunnel.png"),
        textures_path("tunnel/tunnel_back.png"),
        textures_path("tunnel/tunnel_floor_1.png"),
        textures_path("tunnel/tunnel_floor_2.png"),
        textures_path("tunnel/tunnel_floor_3.png"),
        textures_path("kennel_room/cat_lying.png"),
        textures_path("lobby/lobby_back_wall_borderless.png"),
        textures_path("lobby/lobby_floor_top.png"),
        textures_path("lobby/lobby_floor.png"),
        textures_path("lobby/map.png"),
        textures_path("lobby/behind.png"),
        textures_path("start_screen/spaceBackground.png"),
        textures_path("start_screen/startButton.png"),
        textures_path("food/sushi.png"),
        textures_path("objects/key_big.png"),
        textures_path("objects/gem.png"),
        textures_path("lightCone.png"),
        textures_path("start_screen/earthSpriteSheet.png"),
        textures_path("gameLogo.png"),
        textures_path("game_elements/hp_bar_2.png"),
        textures_path("bouncer.png"),
        textures_path("encounter_elements/healthbar.png"),
    	textures_path("encounter_elements/emotion_bar.png"),
        textures_path("encounter_elements/emotion_bar_gauger.png"),
        textures_path("encounter_elements/text_box.png"),
        textures_path("encounter_elements/action_button_2.png"),
        textures_path("Bouncer_left.png"),
        textures_path("objects/money.png"),
        textures_path("food/shrimp.png"),
        textures_path("patrol/johnny004.png"),
        textures_path("patrol/johnny007.png"),
        textures_path("maps/city_map.png"),
        textures_path("rooms/office_room.png"),
        textures_path("rooms/vet_room.png"),
        textures_path("objects/brown_bag.png"),
        textures_path("player/cat_profile.png"),
        textures_path("game_elements/item_box.png"),
        textures_path("encounter_elements/attack_box.png"),
        textures_path("player/black_cat_attack.png"),
        textures_path("encounter_elements/bone.png"),
        textures_path("objects/healer.png"),
        textures_path("objects/med_kit.png"),
        textures_path("objects/flashlight.png"),
        textures_path("npcs/deckard.png"),
        textures_path("npcs/dorio.png"),
        textures_path("npcs/racoon.png"),
        textures_path("food/blue_fish.png"),
        textures_path("game_elements/show_stats.png"),
        textures_path("game_elements/hide_stats.png"),
        textures_path("game_elements/stats_box.png"),
        textures_path("npcs/orange_cat.png"),
        textures_path("npcs/gray_cat.png"),
        textures_path("npcs/dog1_idle.png"),
        textures_path("npcs/beige_cat.png"),
        textures_path("food/mushroom.png"),
        textures_path("food/mushroom2.png"),
        textures_path("food/mushroom3.png"),
        textures_path("objects/gold_necklace.png"),
        textures_path("npcs/raccoon_attack.png"),
        textures_path("encounter_elements/encounter_wall.png"),
        textures_path("encounter_elements/bounce.png"),
        textures_path("encounter_elements/e_press.png"),
        textures_path("npcs/strong_npc_idle.png"),
        textures_path("npcs/mafia_encounter.png"),
        textures_path("rooms/boss_room.png"),
        textures_path("npcs/johnnysilverhand.png"),
        textures_path("npcs/brown_dog.png"),
        textures_path("npcs/brown_dog_attack.png"),
        textures_path("npcs/white_dog.png"),
        textures_path("npcs/brown_black_dog.png"),
        textures_path("npcs/white_brown_dog.png"),
        textures_path("npcs/faraday.png"),
        textures_path("npcs/faraday_attack.png"),
        textures_path("dialogue.png"),
        textures_path("dialogue_box.png"),
        textures_path("select_arrow.png"),
        textures_path("smoke_particle.png"),
        textures_path("encounter_icon.png"),
        textures_path("food/steak.png"),
        textures_path("game_elements/heart.png"),
        textures_path("game_elements/rip.png"),
        textures_path("help_screen.png")
    };

	const std::array<std::string, effect_count> effect_paths = {
		shader_path("black"),
		shader_path("textured"),
        shader_path("backpack")
    };

    std::array<GLuint, effect_count> effects;
    GLuint vao;
    GLuint backpack_vao;
    std::array<GLuint, geometry_count> vertex_buffers;
    std::array<GLuint, geometry_count> index_buffers;
    std::array<Mesh, geometry_count> meshes;

public:
    ~RenderSystem();

    RenderSystem();

    const std::array<GLuint, texture_count>& getTextureHandles() const { return texture_gl_handles; }

    bool init(GLFWwindow* window);

    template <class T>
    void bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices);

    void initializeGlTextures();
    void initializeGlEffects();
    void initializeGlMeshes();
    void initilizeAnimations();
    Mesh& getMesh(GEOMETRY_BUFFER_ID id) { return meshes[(int)id]; };

    void initializeGlGeometryBuffers();

    // Initialize the screen texture used as intermediate render target
    // The draw loop first renders to this texture, then it is used for the wind
    // shader
    bool initScreenTexture();

    void draw(float elapsed_ms_since_last_update, bool is_paused, vec4 overlay_color = {0,0,0,0}, float light_amount = 0.0f);
    void drawOverlayBox(float center_x, float center_y, float box_width, float box_height, glm::vec4 color) const;
    mat3 createProjectionMatrix();
    void drawTexturedMesh(Entity entity, const mat3& projection);
    void setShadowMap(GLuint shadowMap_arg);
    GLFWwindow *getWindow() { return window; }
    GLuint getEffectProgram(EFFECT_ASSET_ID effect) const { return effects[(GLuint)effect]; }

    const std::vector < std::pair<GEOMETRY_BUFFER_ID, std::string>> mesh_paths = {
        std::pair<GEOMETRY_BUFFER_ID, std::string>(GEOMETRY_BUFFER_ID::BACKPACK, std::string(PROJECT_SOURCE_DIR) + "data/meshes/backpack4.obj")
    };

    void setAnimation(RenderRequest& renderRequest, AnimationState state, std::unordered_map<AnimationState, Animation> animationMap);
    std::unordered_map<AnimationState, Animation> catAnimationMap;
    std::unordered_map<AnimationState, Animation> npcAnimationMap;
    std::unordered_map<AnimationState, Animation> dogAnimationMap;

    void drawTransform(float elapsed_ms_since_last_update, const mat3& transform);

    mat3 createCameraProjection(const vec2& cameraPosition);

    mat3 cameraTransform;

private:
    void drawAnimatedMesh(Entity entity, const mat3& projection, float elapsed_ms_since_last_update);
    void drawToScreen(bool is_paused, vec4 overlay_color, float light_amount);
    bool isEntityInView();

    GLFWwindow* window;
    GLuint current_shader;

    GLuint frame_buffer;
    GLuint shadowMap;
    GLuint off_screen_render_buffer_color;
    GLuint off_screen_render_buffer_depth;
    VisualEffectsSystem visualEffectsSystem;

    Entity screen_state_entity;

    void printMat3(const mat3& matrix);
};

bool loadEffectFromFile(const std::string& vs_path, const std::string& fs_path, GLuint& out_program);
