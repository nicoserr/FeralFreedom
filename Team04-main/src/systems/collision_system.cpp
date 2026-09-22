#pragma once

#include "systems/collision_system.hpp"
#include "world/world_system.hpp"
#include <iostream> //for testing
#include "../common.hpp"
#include "mesh_collision.hpp"

CollisionSystem::CollisionSystem() {

}

CollisionSystem::~CollisionSystem() {

}

void CollisionSystem::step(LevelSystem* levelManager) {
	detectAABB(levelManager);
	detectCone();
	//detectMouse(registry); TODO
}

void CollisionSystem::detectAABB(LevelSystem* ls) {
	std::vector<Entity> colliders_list = ls->currentLevel->currentRoom->non_rendered_entities;
	for (Entity collider : colliders_list) {
		if (registry.colliders.has(collider)) {
			COLLIDER_TYPE type = registry.colliders.get(collider).type;
			if (type == PLAYER) {
				for (Entity non_player : colliders_list) {
					if (registry.colliders.has(non_player))
					{
						if (collides(collider, non_player)) {
							//resolve the collision
							handlePlayerCollision(collider, non_player, ls);
							// consider creating a collision component to store this information
							// and offload collision handling to somewhere else
						}
					}
				}
			}
			else if (type == CREATURE || type == PATROL) {
				for (Entity collidee : colliders_list) {
					if (registry.colliders.has(collidee))
					{
						if (collides(collider, collidee)) {
							//resolve the collision
							handleCollision(collider, collidee, ls);
							// consider creating a collision component to store this information
							// and offload collision handling to somewhere else
						}
					}
				}
			}
		}
	}
}

bool CollisionSystem::collides(Entity a, Entity b) {
	if (a == b)
		return false;

	vec2 position_a = registry.motions.get(a).position + registry.boundingBoxes.get(a).offset;
	vec2 position_b = registry.motions.get(b).position + registry.boundingBoxes.get(b).offset;
	BoundingBox& bb_a = registry.boundingBoxes.get(a);
	BoundingBox& bb_b = registry.boundingBoxes.get(b);
	float width_a = bb_a.width;
	float width_b = bb_b.width;
	float height_a = bb_a.height;
	float height_b = bb_b.height;

	/* this procedure assumes that the bounding box is centered on the position vector
	x axis goes from left to right, y axis goes from bottom to top*/
	if (position_a.x + width_a / 2 <= position_b.x - width_b / 2)
		return false;
	if (position_a.x - width_a / 2 >= position_b.x + width_b / 2)
		return false;
	if (position_a.y + height_a / 2 <= position_b.y - height_b / 2)
		return false;
	if (position_a.y - height_a / 2 >= position_b.y + height_b / 2)
		return false;
	
	return true;
}

void CollisionSystem::handlePlayerCreature(Entity player, Entity creature) {
	if (!registry.npcs.has(creature))
		return;
  
	handleCreatureObstacle(player, creature);
}

void CollisionSystem::handlePlayerPatrol(Entity player_entity, Entity patrol) {
	if (!registry.players.has(player_entity) || !registry.patrols.has(patrol))
		return;
	Player& player = registry.players.get(player_entity);
	Stats& playerStats = registry.stats.get(player_entity);

	if (player.lives > 0 && !player.lost_life) {
		player.lives--;
		player.lost_life = true;
		player.lost_life_timer_ms = 3000.f;
		playerStats.currentHp = playerStats.maxHp;
		registry.deathTimers.emplace(player_entity);
	}
	else if (player.lives <= 0 && !registry.deathTimers.has(player_entity)) {
		player.lost_life = false;
		game_over = true;
	}
}

void CollisionSystem::handleCreatureObstacle(Entity creature, Entity obstacle) {
	// For random walker–obstacle collisions
	bool is_rw_collision = registry.randomWalkers.has(creature);
	vec2 tmp_vel;
	float tmp_angle;
	if (is_rw_collision) {
		tmp_vel = registry.motions.get(creature).velocity;
		tmp_angle = registry.motions.get(creature).angle;
	}

	Motion& creature_motion = registry.motions.get(creature); //creature motion
	Motion& obstacle_motion = registry.motions.get(obstacle); //obstacle motion
	BoundingBox& creature_bb = registry.boundingBoxes.get(creature); //creature bounding box
	BoundingBox& obstacle_bb = registry.boundingBoxes.get(obstacle); //obstacle bounding box

	// ejecting creature from the wall
	vec2 c_pos = creature_bb.offset + creature_motion.position;
	float c_width = creature_bb.width;
	float c_height = creature_bb.height;

	vec2 o_pos = obstacle_bb.offset + obstacle_motion.position;
	float o_width = obstacle_bb.width;
	float o_height = obstacle_bb.height;

	// we want to calculate how much the two boxes overlap
	float x_overlap = min(abs((o_pos.x + o_width / 2.f) - (c_pos.x - c_width / 2.f)), abs((c_pos.x + c_width / 2.f) - (o_pos.x - o_width / 2.f)));
	float y_overlap = min(abs((o_pos.y+ o_height / 2.f) - (c_pos.y - c_height / 2.f)), abs((c_pos.y + c_height / 2.f) - (o_pos.y - o_height / 2.f)));

	// we have a box, we have a vector, we can calculate the minimum amount of times we need
	// to apply this vector to the collider to make one of the overlaps go away
	vec2 movement = creature_motion.velocity;
	float x_multiplier, y_multiplier;
	if (movement.x == 0){
		x_multiplier = 99999.f;
	} else {
		x_multiplier = x_overlap / movement.x;
	}
	if (movement.y == 0) {
		y_multiplier = 99999.f;
	} else {
		y_multiplier = y_overlap / movement.y;
	}
	float t = min(abs(x_multiplier), abs(y_multiplier));


	if (abs(y_multiplier) < abs(x_multiplier)) {
		creature_motion.position.y -= t * movement.y;
	} else if (abs(y_multiplier) > abs(x_multiplier)) {
		creature_motion.position.x -= t * movement.x;
	} else {
		creature_motion.position -= t* movement;
	}

	/*
	if (t != 99999.f) {
		motion_c.position += -1.f* t * movement;
	} */
	/*
	if (abs(y_overlap) > abs(x_overlap)) {
		motion_c.velocity.x = 0;
	} else if (abs(y_overlap) < abs(x_overlap)) {
		motion_c.velocity.y = 0;
	} else {
		motion_c.velocity = {0,0};
	}
	*/

	// update random walker velocity and angle
	if (is_rw_collision) {
		creature_motion.velocity = -tmp_vel;
		creature_motion.angle = tmp_angle + M_PI;
	}

}

void CollisionSystem::handlePlayerDoor(Entity player, Entity door, LevelSystem* ls) {
	//std::cout << ls->levels.size() << std::endl;
	if(!registry.keyInventory.has(player)){
		return;
	}

	KeyInventory& keyInven = registry.keyInventory.get(player);

	if(!registry.doors.get(door).is_open && keyInven.keys.size() < registry.doors.get(door).required_keys){
		handleCreatureObstacle(player, door);
		return;
	} else if(keyInven.keys.size() >= registry.doors.get(door).required_keys){

		registry.doors.get(door).is_open = true;
	}

	Level* level = ls->currentLevel;
	Room* room = level->currentRoom;
	//ls->cleanUpCurrentRoom();

	// Find door collided with
	int door_id = door;

	// Update the spawn position in current room for re entry
	auto newPositionIT = level->spawnPositions.find(door_id);
	if (newPositionIT != level->spawnPositions.end()) {
		room->player_position = newPositionIT->second;
		// std::cout << newPositionIT->second.x << "," << newPositionIT->second.y << std::endl;
		// std::cout << "Updated spawn for re entry" << std::endl;
		// std::cout << "Updated Level pointer: " << &level << std::endl;
	}

	auto newDirectionIT = level->spawnDirections.find(door_id);
	TEXTURE_ASSET_ID t = newDirectionIT->second;

	// Get room connected to the doorway
	auto connectedRoomsIT = level->connectedRooms.find(door_id);
	Room* next_room = connectedRoomsIT->second;

	registry.renderRequests.clear();

	ls->currentLevel->currentRoom = next_room;

	RenderRequest& r = next_room->entity_render_requests[player];
	r.used_texture = t;

	ls->renderCurrentRoom(player);
}

// TODO: delete if not necessary
/*
void CollisionSystem::handleDoorCollision(Entity collider, Entity collidee, LevelSystem* ls) {
	//std::cout << ls->levels.size() << std::endl;
	if(!registry.players.has(collider) || !registry.doors.get(collidee).is_open) {
		return;
	}
	Level* level = ls->currentLevel;
	Room* room = level->currentRoom;
	//ls->cleanUpCurrentRoom();

	// Find door collided with
	int door_id = collidee;

	// Update the spawn position in current room for re entry
	auto newPositionIT = level->spawnPositions.find(door_id);
    if (newPositionIT != level->spawnPositions.end()) {
        room->player_position = newPositionIT->second;
		// std::cout << newPositionIT->second.x << "," << newPositionIT->second.y << std::endl;
		// std::cout << "Updated spawn for re entry" << std::endl;
		// std::cout << "Updated Level pointer: " << &level << std::endl;
    }

	auto newDirectionIT = level->spawnDirections.find(door_id);
	TEXTURE_ASSET_ID t = newDirectionIT->second;

	// Get room connected to the doorway
	auto connectedRoomsIT = level->connectedRooms.find(door_id);
	Room* next_room = connectedRoomsIT->second;

	registry.renderRequests.clear();

	ls->currentLevel->currentRoom = next_room;

	RenderRequest& r = next_room->entity_render_requests[collider];
	r.used_texture = t;

	ls->renderCurrentRoom(collider);
}
*/

//Handles what happens when two things do collide, consider creating a collision component
//for handling collision instead of just this one function
void CollisionSystem::handleCollision(Entity creature, Entity collidee, LevelSystem* ls) {
	//stub, fill this in later for actual behaviour
	//std::cout << "COLLISION_SYSTEM: Collision detected between entity id: " << collider << " and " << collidee << std::endl;
	if(!ls->currentLevel->currentRoom->isEntityInRoom(collidee) || !ls->currentLevel->currentRoom->isEntityInRoom(collidee)) {
		return;
	}
	COLLIDER_TYPE collidee_type = registry.colliders.get(collidee).type;

	switch (collidee_type) {
		case CREATURE:
			// handleCreatureCreature(creature, collidee);
			break;
		case OBSTACLE:
			//only behaviour we are expecting is hitting a wall
			handleCreatureObstacle(creature, collidee);
			break;
		case FRICTION:
			if (registry.players.has(creature))
			{
				handleFrictionCollision(creature, collidee);
			} else
			{
				handleCreatureObstacle(creature, collidee);
			}
			break;
		default:
			break;
	}
}

void CollisionSystem::handlePlayerCollision(Entity player, Entity non_player, LevelSystem* ls) {
	switch (registry.colliders.get(non_player).type)
	{
	case CREATURE:
		handlePlayerCreature(player, non_player);
		break;
	case PATROL:
		handlePlayerPatrol(player, non_player);
		break;
	case OBSTACLE:
		// same behaviour
		handleCreatureObstacle(player, non_player);
		break;
	case ITEM:
		handlePlayerItem(player, non_player, ls);
		break;
	case DOOR:
		handlePlayerDoor(player, non_player, ls);
		break;
	case FRICTION:
		handleFrictionCollision(player, non_player);
		break;
	default:
		break;
	}
}

//Note: Assumes that in Playstate update, motion.speedMod is reset to 1.0
void CollisionSystem::handleFrictionCollision(Entity collider, Entity collidee)
{
	if (!registry.colliders.has(collidee)) {
		return;
	}
	Collider& f_collider = registry.colliders.get(collidee);

	if (registry.motions.has(collider)) {
		Motion& motion = registry.motions.get(collider);
		motion.speedMod -= f_collider.friction;
		if (motion.speedMod > 1.0f)
			motion.speedMod = 1.0f;
		if (motion.speedMod < 0.0f)
			motion.speedMod = 0.0f;
	}
}

void CollisionSystem::handlePlayerItem(Entity player, Entity item, LevelSystem* ls){
	if (!registry.meshPtrs.has(item))
	{
		return;
	}
	const Mesh* collidee_mesh = registry.meshPtrs.get(item);
    std::vector<vec2> transformed_collidee_vertices;
    mat3 collidee_transform = createTransformMatrix(registry.motions.get(item));
    for (const auto& vertex : collidee_mesh->vertices) {
        vec3 transformed_vertex = collidee_transform * vec3(vertex.position.x, vertex.position.y, 1.0f);
        transformed_collidee_vertices.emplace_back(transformed_vertex.x, transformed_vertex.y);
    }

	// get OBB vertices for the bounding box of the player
	Motion& player_motion = registry.motions.get(player);
	BoundingBox& player_bbox = registry.boundingBoxes.get(player);
	std::vector<vec2> corners = calculateOBBVertices(player_motion, player_bbox); 
	for (size_t i = 0; i < transformed_collidee_vertices.size() - 2; i +=3)
	{
		if (MeshCollision::checkSATCollision(corners, {transformed_collidee_vertices[i], 
													transformed_collidee_vertices[i + 1], transformed_collidee_vertices[i + 2]}))
		{
			
			Room* room = ls->currentLevel->currentRoom;
			room->entity_render_requests.erase(item);

			if (registry.renderRequests.has(item)) {
				registry.renderRequests.remove(item);
			}

			auto it = std::find(room->rendered_entities.begin(), room->rendered_entities.end(), item);
			if (it != room->rendered_entities.end()) {
				room->rendered_entities.erase(it);
			}

			it = std::find(room->non_rendered_entities.begin(), room->non_rendered_entities.end(), item);
			if (it != room->non_rendered_entities.end()) {
				room->non_rendered_entities.erase(it);
			}
			
			if (registry.consumableItems.has(item)) {
				registry.consumableItems.remove(item);
			}

			if (room->meshToTextureMap.count(item) > 0) {
				Entity textureEntity = room->meshToTextureMap[item];
				if (registry.renderRequests.has(textureEntity)) {
					registry.renderRequests.remove(textureEntity);
				}
				auto it = std::find(room->rendered_entities.begin(), room->rendered_entities.end(), textureEntity);
				if (it != room->rendered_entities.end()) {
					room->rendered_entities.erase(it);
				}
			}
		}
	}
}

std::vector<vec2> CollisionSystem::calculateOBBVertices(const Motion& motion, BoundingBox& bbox) {
    vec2 half_box = vec2 {bbox.width, bbox.height};

    // define corners relative to the center
    std::vector<vec2> corners = {
        { half_box.x, half_box.y }, // top-right corner
        { half_box.x, -half_box.y },  // bottom-right corner
        { -half_box.x, -half_box.y },  // bottom-left corner
        { -half_box.x, half_box.y }  // top-left corner
    };

    // rotate each corner around the entity's position based on motion.angle
    mat2 rotation_matrix = mat2(cos(motion.angle), sin(motion.angle), -sin(motion.angle), cos(motion.angle));
    for (auto& corner : corners) {
        corner = rotation_matrix * corner + motion.position;
    }
    // returns rotated bounding box corners
    return corners;
}

mat3 CollisionSystem::createTransformMatrix(const Motion &motion) {
    mat3 translation = { {1, 0, 0}, {0, 1, 0}, {motion.position.x, motion.position.y, 1} };
    mat3 rotation = { {cos(motion.angle), sin(motion.angle), 0}, {-sin(motion.angle), cos(motion.angle), 0}, {0, 0, 1} };
    mat3 scale = { {motion.scale.x, 0, 0}, {0, motion.scale.y, 0}, {0, 0, 1} };
    return translation * rotation * scale;
}

// this function sets the chasing_player flag to true for all patrols if one patrol has the player
// within their cone of detection. Treats the player like a point, not an AABB.
// Consider modifying this for modularity
void CollisionSystem::detectCone() {
	if (registry.players.entities.size() == 0)
		return; //guard, in case player hasn't been created yet

	float coneAngle = M_PI/3;
	Entity player = registry.players.entities[0]; //hard coded to get the player, sketchy stuff
	vec2 player_pos = registry.motions.get(player).position;
	for (Entity patrol : registry.patrols.entities) {
		registry.patrols.get(patrol).player_seen = false;
		vec2 patrol_pos = registry.motions.get(patrol).position;
		float patrol_angle = registry.motions.get(patrol).angle;
		//std::cout << patrol_angle << std::endl;
		float rad_angle = patrol_angle;
		//std::cout << rad_angle << std::endl;

		float half_cone_angle = coneAngle/2.0f;
		//std::cout << half_cone_angle << std::endl;

		vec2 direction = { cos(rad_angle), sin(rad_angle)};
		vec2 left_edge = {
			cos(rad_angle - half_cone_angle),
			sin(rad_angle - half_cone_angle)
		};
		vec2 right_edge = {
			cos(rad_angle + half_cone_angle),
			sin(rad_angle + half_cone_angle)
		};

		Patrol& curr_patrol = registry.patrols.get(patrol);

		if(playerInCone(player_pos, patrol_pos, direction, left_edge, right_edge)) {
			// std::cout << "Detected Player!" << std::endl;
			curr_patrol.player_seen = true;
		} else {
			curr_patrol.player_seen = false;
		}

	}
}

bool CollisionSystem::playerInCone(vec2 player_pos, vec2 patrol_pos, vec2 direction, vec2 left_edge, vec2 right_edge) {
	vec2 to_player = {player_pos.x - patrol_pos.x, player_pos.y - patrol_pos.y};
	float distance_to_player_squared = to_player.x * to_player.x + to_player.y * to_player.y;

	if (distance_to_player_squared > MAX_DETECTION_DISTANCE_SQUARED) {
		return false;
	}


	direction = normalize(direction);
	to_player = normalize(to_player);


	float cone_cos = cos(M_PI / 6);
	float dot_product = dot(direction, to_player);

	return dot_product >= cone_cos;
}

float dot(const vec2& a, const vec2& b) {
	return a.x * b.x + a.y * b.y;
}

vec2 normalize(vec2 v) {
	float l = std::sqrt(v.x * v.x + v.y * v.y);
	if(l > 0) {
		v.x /= l;
		v.y /= l;
	}

	return {v.x, v.y};
}

// TODO: Maybe useless
/*
void CollisionSystem::handleCreatureCreature(Entity collider, Entity collidee) {
	bool patrol_player_collision =
		(registry.players.has(collidee) && registry.patrols.has(collider)) ||
		(registry.players.has(collider) && registry.patrols.has(collidee));
	bool encounter_npc_player_collision = (registry.players.has(collidee) && registry.npcs.has(collider) && registry.npcs.get(collider).isInteractable);
	bool encounter_player_npc_collision = (registry.players.has(collider) && registry.npcs.has(collidee) && registry.npcs.get(collidee).isInteractable);

	if (patrol_player_collision)
	{
		registry.deathTimers.emplace(collidee);
	}
	else if (encounter_npc_player_collision) {
		Entity npc_entity = collider;
		NPC& npc = registry.npcs.get(npc_entity);
		npc.start_encounter = true;
	}
	else if (encounter_player_npc_collision)
	{
		Entity npc_entity = collidee;
		NPC& npc = registry.npcs.get(npc_entity);
		npc.start_encounter = true;
	}
}
*/