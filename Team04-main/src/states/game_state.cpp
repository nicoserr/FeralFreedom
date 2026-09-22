#include "game_state.hpp"
#include "core/ecs_registry.hpp"
#include "core/ecs.hpp"
#include "world/world_system.hpp"

Entity GameState::createUIItem(vec2 position, vec2 size, TEXTURE_ASSET_ID texture_id) {
    Entity item = registry.create_entity();
    Motion& motion = registry.motions.emplace(item);
    motion.position = position;
    motion.scale = size;

    registry.renderRequests.insert(item, {
        texture_id,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE
    });
    return item;
}

Entity GameState::createBackground(TEXTURE_ASSET_ID texture_id) {
    // Create a new background entity
    background = registry.create_entity();

    Motion &motion = registry.motions.emplace(background);
    motion.position = vec2(window_width_px / 2, window_height_px / 2);
    motion.scale = vec2(window_width_px, window_height_px);

    registry.renderRequests.insert(background, {
                                       texture_id,
                                       EFFECT_ASSET_ID::TEXTURED,
                                       GEOMETRY_BUFFER_ID::SPRITE,
                                       {},
                                       false
                                   });
    return background;
}

myButton GameState::createButton(vec2 position, vec2 size, TEXTURE_ASSET_ID texture_id, BUTTON_TYPE button_type) {
    Entity button_entity = registry.create_entity();
    Motion &motion = registry.motions.emplace(button_entity);
    motion.position = position;
    motion.scale = size;

    registry.renderRequests.insert(button_entity, {
                                       texture_id,
                                       EFFECT_ASSET_ID::TEXTURED,
                                       GEOMETRY_BUFFER_ID::SPRITE,
                                       {},
                                       false
                                   });

    myButton button = { button_entity, position, size, button_type };
    return button;
}

bool GameState::is_click_inside_button(const vec2& mouse_pos, const myButton& button, WorldSystem& game) {
    int window_width, window_height;
    int framebuffer_width, framebuffer_height;

    vec2 button_pos = button.position;
    vec2 button_size = button.size;
    glfwGetWindowSize(game.get_window(), &window_width, &window_height);
    glfwGetFramebufferSize(game.get_window(), &framebuffer_width, &framebuffer_height);

    // Calculate the pixel density ratio --> this is needed for it to work on both macs and windows
    //float pixel_ratio_x = static_cast<float>(framebuffer_width) / window_width;
    //float pixel_ratio_y = static_cast<float>(framebuffer_height) / window_height;

    // Calculate button boundaries
    float left_bound = button_pos.x - (button_size.x/2);
    float right_bound = button_pos.x + (button_size.x/2);
    float top_bound = button_pos.y - (button_size.y/2) ;
    float bottom_bound = button_pos.y + (button_size.y/2);

    // Check if mouse is inside the button's boundaries
    return (mouse_pos.x >= left_bound && mouse_pos.x <= right_bound &&
            mouse_pos.y >= top_bound && mouse_pos.y <= bottom_bound);
}


void GameState::promptSaveOnExit(WorldSystem* game) {
    const vec4 color = { 0.0f, 0.0f, 0.0f, 0.5f }; // Semi-transparent black
    float box_width = window_width_px * 0.4f;
    float box_height = window_height_px * 0.2f;
    float center_x = window_width_px / 2.0f;
    float center_y = window_height_px / 2.0f;

    // Draw the overlay box for pause menu
    game->get_renderer()->drawOverlayBox(center_x, center_y, box_width, box_height, color);
    game->get_renderer()->draw(0, true);

    game->get_text_renderer()->RenderCenteredText("Save before exiting?", window_width_px, window_height_px / 2 + 30, 0.6, {1.0, 1.0, 1.0});
    game->get_text_renderer()->RenderCenteredText("[Y] Yes   [N] No", window_width_px, window_height_px / 2 - 10, 0.4, {1.0, 1.0, 1.0});
    game->get_text_renderer()->RenderCenteredText("[C] Continue Playing", window_width_px, window_height_px / 2 - 50, 0.4, {1.0, 1.0, 1.0});

    glDisable(GL_BLEND);
}


Entity GameState::createHealthBar() {
    Entity entity = registry.create_entity();

    Motion &motion = registry.motions.emplace(entity);
    motion.position = {window_width_px - window_width_px / 12, window_height_px / 12};
    motion.scale = {150, 150};

    registry.uiElements.emplace(entity);

    RenderRequest renderRequest;
    renderRequest.used_texture = TEXTURE_ASSET_ID::HP_BAR;
    renderRequest.used_effect = EFFECT_ASSET_ID::TEXTURED;
    renderRequest.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    renderRequest.hasAnimation = true;
    renderRequest.animation.frameCount = 14;
    renderRequest.animation.currentFrame = 0;
    renderRequest.animation.frameTime = 0.0f;
    renderRequest.animation.elapsedTime = 0.0f;
    renderRequest.animation.columns = 14;
    renderRequest.animation.rows = 1;
    renderRequest.animation.startRow = 0;
    renderRequest.animation.startCol = 0;

    registry.renderRequests.insert(entity, renderRequest);

    return entity;
}

void GameState::updateHealthBar(Entity player) {
    if (registry.stats.has(player) && registry.motions.has(health_bar)) {
        Stats& playerStats = registry.stats.get(player);
        Motion& healthMotion = registry.motions.get(health_bar);
        RenderRequest& request = registry.renderRequests.get(health_bar);

        float current_health_percentage = static_cast<float>(playerStats.currentHp) / static_cast<float>(playerStats.maxHp);
        // Update the animation frame based on the percentage
        int frameIndex = static_cast<int>((1.0f - current_health_percentage) * (request.animation.frameCount - 1));
        frameIndex = clamp(frameIndex, 0, request.animation.frameCount - 1);
        request.animation.currentFrame = frameIndex;
    }
}

Entity GameState::createPlayerProfile() {
    Entity entity = registry.create_entity();

    Motion &motion = registry.motions.emplace(entity);
    motion.position = {window_width_px - window_width_px / 10, window_height_px / 15};
    motion.scale = {100, 100};

    registry.uiElements.emplace(entity);

    RenderRequest renderRequest;
    renderRequest.used_texture = TEXTURE_ASSET_ID::BLACK_CAT_PORTRAIT;
    renderRequest.used_effect = EFFECT_ASSET_ID::TEXTURED;
    renderRequest.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    renderRequest.hasAnimation = false;
    renderRequest.animation = {};

    registry.renderRequests.insert(entity, renderRequest);
    return entity;
}

Entity GameState::createInventory() {
    Entity inventory_box_1 = registry.create_entity();
    Motion &inventory_motion = registry.motions.emplace(inventory_box_1);
    inventory_motion.position = {window_width_px - window_width_px / 9, window_height_px / 5};
    inventory_motion.scale = {70, 70};
    registry.uiElements.emplace(inventory_box_1);

    RenderRequest inventory_render_request;
    inventory_render_request.used_texture = TEXTURE_ASSET_ID::ITEM_BOX;
    inventory_render_request.used_effect = EFFECT_ASSET_ID::TEXTURED;
    inventory_render_request.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
    inventory_render_request.hasAnimation = false;
    inventory_render_request.animation = {};
    registry.renderRequests.insert(inventory_box_1, inventory_render_request);

    Entity inventory_box_2 = registry.create_entity();
    Motion &inventory_motion_2 = registry.motions.emplace(inventory_box_2);
    inventory_motion_2 = inventory_motion;
    inventory_motion_2.position = {window_width_px - window_width_px / 15, window_height_px / 5};
    RenderRequest inventory_render_request_2 = inventory_render_request;
    registry.uiElements.emplace(inventory_box_2);
    registry.renderRequests.insert(inventory_box_2, inventory_render_request_2);

    return inventory_box_1;
}

void GameState::showStats(const Stats &playerStats, WorldSystem* game, vec2 position) {
    if (!textRenderer) {
        textRenderer = game->get_text_renderer();
    }
    vec2 boxPosition = position;
    textRenderer->RenderText("Reputation: " + std::to_string(static_cast<int>(playerStats.reputation)), boxPosition.x, boxPosition.y,
                             defaultTextScale, {1, 1, 1});
    textRenderer->RenderText("Cuteness: " + std::to_string(static_cast<int>(playerStats.cuteness)), boxPosition.x, boxPosition.y + 20,
                             defaultTextScale, {1, 1, 1});
    textRenderer->RenderText("Ferocity: " + std::to_string(static_cast<int>(playerStats.ferocity)), boxPosition.x, boxPosition.y + 40,
                             defaultTextScale, {1, 1, 1});
    textRenderer->RenderText("Agility: " + std::to_string(static_cast<int>(playerStats.agility)), boxPosition.x, boxPosition.y + 60,
                             defaultTextScale, {1, 1, 1});
}

void GameState::drawUIElements(WorldSystem* game, Entity player) {
    if (!registry.renderRequests.has(player_profile)) {
        registry.remove_all_components_of(player_profile);
        player_profile = createPlayerProfile();
    }

    if (!registry.renderRequests.has(health_bar)) {
        registry.remove_all_components_of(health_bar);
        health_bar = createHealthBar();
    }
    updateHealthBar(player_character);

    if (heartEntities.empty()) {
        createHearts();
    }
    renderHearts(registry.players.get(player_character).lives);

    if (!registry.renderRequests.has(inventory)) {
        registry.remove_all_components_of(inventory);
        inventory = createInventory();
    }
    drawEquipped(player_character);

    if (show_stats) {  // Showing the stats
        showStats(registry.stats.get(player_character), game, { window_width_px - UI_tile_x * 6.5, window_height_px - UI_tile_y * 24});
        if (registry.renderRequests.has(showStatsButton.entity)) {
            registry.remove_all_components_of(showStatsButton.entity);
        }
        if (!registry.uiElements.has(hideStatsButton.entity)) {
            registry.uiElements.emplace(hideStatsButton.entity);
        }
        if (!registry.renderRequests.has(hideStatsButton.entity)) {
            registry.remove_all_components_of(hideStatsButton.entity);
            hideStatsButton = createButton({window_width_px - UI_tile_x * 4, UI_tile_y * 18}, {UI_tile_x * 6, UI_tile_y * 2},
                       TEXTURE_ASSET_ID::HIDE_STATS_BUTTON, BUTTON_TYPE::SHOW_STATS);
        }
        if (!registry.renderRequests.has(stats_box)) {
            stats_box = registry.create_entity();
            Motion &motion = registry.motions.emplace(stats_box);
            motion.position = {window_width_px - UI_tile_x * 4, UI_tile_y * 23};
            motion.scale = {UI_tile_x * 6, UI_tile_y * 9};
            registry.uiElements.emplace(stats_box);
            registry.renderRequests.insert(stats_box, {
                                           TEXTURE_ASSET_ID::STATS_BOX,
                                           EFFECT_ASSET_ID::TEXTURED,
                                           GEOMETRY_BUFFER_ID::SPRITE,
                                           {},
                                           false
                                       });
        }
    } else {  // Hiding the stats
        if (registry.renderRequests.has(hideStatsButton.entity)) {
            registry.remove_all_components_of(hideStatsButton.entity);
        }
        if (!registry.uiElements.has(showStatsButton.entity)) {
            registry.uiElements.emplace(showStatsButton.entity);
        }
        if (!registry.renderRequests.has(showStatsButton.entity)) {
            registry.remove_all_components_of(showStatsButton.entity);
            showStatsButton = createButton({window_width_px - UI_tile_x * 4, UI_tile_y * 18}, {UI_tile_x * 6, UI_tile_y * 2},
                                       TEXTURE_ASSET_ID::SHOW_STATS_BUTTON, BUTTON_TYPE::SHOW_STATS);
        }
        if (registry.renderRequests.has(stats_box)) {
            registry.remove_all_components_of(stats_box);
        }
    }
}

// Create the hearts for the player's lives to avoid creating entities in every frame.
void GameState::createHearts() {
    for (int i = 0; i < maxLives; ++i) {
        Entity heart = registry.create_entity();
        heartEntities.push_back(heart);
    }
}

void GameState::renderHearts(int lives) {
    int xPos = window_width_px - UI_tile_x * 10;
    int yPos = UI_tile_y * 4;

    for (int i = 0; i < maxLives; ++i) {
        Entity heart = heartEntities[i];
        // Create the motion component if it doesn't exist.
        // This case happens after we come back from encounter state.
        if (!registry.motions.has(heart)) {
            Motion &heartMotion = registry.motions.emplace(heart);
            heartMotion.position = {xPos, yPos};
            heartMotion.scale = {UI_tile_x * 1.5, UI_tile_y * 1.5};
            heartMotion.z = 1.0f;
            xPos += UI_tile_x * 1.5;
        }

        // Remove the previous render requests and add the new ones
        if (registry.renderRequests.has(heart)) {
            registry.renderRequests.remove(heart);
        } else if (!registry.uiElements.has(heart)) {
            registry.uiElements.emplace(heart);
        }

        RenderRequest heartRenderRequest;
        heartRenderRequest.used_texture = TEXTURE_ASSET_ID::HEART;
        heartRenderRequest.used_effect = EFFECT_ASSET_ID::TEXTURED;
        heartRenderRequest.used_geometry = GEOMETRY_BUFFER_ID::SPRITE;
        heartRenderRequest.hasAnimation = true;

        heartRenderRequest.animation.frameCount = 2;
        heartRenderRequest.animation.currentFrame = 0;
        heartRenderRequest.animation.frameTime = 0.0f;
        heartRenderRequest.animation.elapsedTime = 0.0f;
        heartRenderRequest.animation.columns = 2;
        heartRenderRequest.animation.rows = 1;
        heartRenderRequest.animation.startRow = 0;
        heartRenderRequest.animation.startCol = (i < lives) ? 0 : 1; // Column 0 for full, 1 for empty
        registry.renderRequests.insert(heart, heartRenderRequest);
    }
}


void GameState::drawEquipped(Entity player) {
    const Inventory& owned_items = registry.inventory.get(player);
    int iconSize = 50;
    vec2 box_position= registry.motions.get(inventory).position;
    int xPos = box_position.x;
    int yPos = box_position.y;

    for (Entity item : owned_items.items) {
        Motion& itemMotion = registry.motions.get(item);
        itemMotion.position = { xPos, yPos };
        itemMotion.scale = { iconSize, iconSize };
        xPos += iconSize + UI_tile_y * 2;
        itemMotion.z = 1.0f;
        if (!registry.uiElements.has(item)) {
            registry.uiElements.emplace(item);
        }
        if (!registry.renderRequests.has(item)) {
            EquippableItem& equippable = registry.equippableItems.get(item);
            TEXTURE_ASSET_ID texture = equippable.texture;
            registry.renderRequests.insert(item, {
                                               texture,
                                               EFFECT_ASSET_ID::TEXTURED,
                                               GEOMETRY_BUFFER_ID::SPRITE,
                                               {},
                                               false
                                           });
        }
    }
}


void GameState::buryPlayer(Entity player, Room* currentRoom) {};

