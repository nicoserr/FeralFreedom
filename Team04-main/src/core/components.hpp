#pragma once
#include "common.hpp"
#include <vector>
#include <queue>
#include <string>
#include "../ext/stb_image/stb_image.h"

struct Player
{
    int lives = 3;
    bool lost_life = false;
    float lost_life_timer_ms = 0.f; // immune to damage after losing a life for a short period
    std::string name = "Player";
};

enum class AnimationState {
    MOVING_LEFT,
    MOVING_RIGHT,
    MOVING_UP,
    MOVING_DOWN,
    MOVING_UP_LEFT,
    MOVING_UP_RIGHT,
    MOVING_DOWN_LEFT,
    MOVING_DOWN_RIGHT,
    SITTING,
    SLEEPING,
    LAYING_DOWN,
    ON_TWO_FEET,
    IDLE,
};

struct Stats
{
    int maxHp = 0;
    int currentHp = 0;
    int stamina = 0;
    float ferocity = 0; // we use ferocity for npc damage
    float cuteness = 0; // we use cuteness for npc charm threshold
    float agility = 0; // we use agility for npc bonus to-hit
    float stealth = 0;
    float reputation = 0; // we use reputation for npc intimidate threshold
    float intelligence = 0;
};

/* We might want to save the reward after an encounter as a componet of the npc
struct Loot
{

};
*/

struct Inventory
{
    std::vector<Entity> items;
};

struct BoundingBox
{
    float width;
    float height;
    vec2 offset = {0, 0};
};

struct Motion {
    vec2 position = {0, 0};
    float angle = 0;
    vec2 velocity = {0, 0};
    vec2 scale = {10, 10};
    float speed;
    float speedMod = 1.0f;
    float z = 0;
};

struct Door
{
    bool is_open = true;
    int required_keys = 0;
};

enum COLLIDER_TYPE {
    PLAYER = 0,
    PATROL = PLAYER + 1,
    CREATURE = PATROL + 1,
    OBSTACLE = CREATURE + 1,
    ITEM = OBSTACLE + 1,
    DOOR = ITEM + 1,
    FRICTION,
    COLLIDERS_SIZE
};

struct Collider
{
    COLLIDER_TYPE type;
    bool transparent = false;
    float friction = 0.0f;
};

struct Patrol
{
    bool player_seen; // a bool updated by collision system, true if the player is within the cone
    Entity light;
};

struct SetMotion
{
    float timer = 100;
};

struct Hidden
{
    bool hidden;
};

/*
struct Stats
{
    int hp = 0;
    int maxHp = 0;
    float stamina = 0;
    float intelligence = 0;
    float cuteness = 0;
    float ferocity = 0;
    float stealth = 0;
    float reputation = 0;
};
*/

struct ConsumableItem
{
    Stats statModifiers;
    bool isBackpack  = false;
    bool isInteractable = false;
};



// steps_to_turn are the steps being taken before turning again
struct RandomWalker {
    float sec_since_turn = 0;
    float sec_to_turn = 0;
};

struct UIElement {
};

struct Rotatable {
};

// For entities converging on target using A*
struct Converger
{
    vec2 target_pos;
    std::vector<vec2> path_to_target;
    bool path_found = false;
    float counter_ms;
};

// For entities converging on target directly
struct Chaser
{
    vec2 target_pos;
    float counter_ms;
};

struct Animation {
    int frameCount;
    int currentFrame;
    float frameTime;
    float elapsedTime;
    int columns;         // Total number of columns in the sprite sheet
    int rows;            // Total number of rows in the sprite sheet
    int startRow;        // Row where the animation starts
    int startCol;     // Column where the animation starts
    AnimationState current_state;
};

// Single Vertex Buffer element for non-textured meshes
struct ColoredVertex
{
	vec3 position;
	vec3 color;
};

// Single Vertex Buffer element for textured sprites (textured.vs.glsl)
struct TexturedVertex
{
	vec3 position;
	vec2 texcoord;
};

struct Mesh
{
	static bool loadFromOBJFile(std::string obj_path, std::vector<ColoredVertex>& out_vertices, std::vector<uint16_t>& out_vertex_indices, vec2& out_size);
	vec2 original_size = {1,1};
	std::vector<ColoredVertex> vertices;
	std::vector<uint16_t> vertex_indices;
};

enum class TEXTURE_ASSET_ID {
    BLACK_CAT_SPRITE_SHEET = 0,
    CAT_DOWN = BLACK_CAT_SPRITE_SHEET + 1,
    CAT_RIGHT = CAT_DOWN + 1,
    CAT_LEFT = CAT_RIGHT + 1,
    CAT_UP = CAT_LEFT + 1,
    CAT_DOWN_LEFT = CAT_UP + 1,
    CAT_DOWN_RIGHT = CAT_DOWN_LEFT + 1,
    CAT_UP_LEFT = CAT_DOWN_RIGHT + 1,
    CAT_UP_RIGHT = CAT_UP_LEFT + 1,
    PATROL_DOWN = CAT_UP_RIGHT + 1,
    PATROL_LEFT = PATROL_DOWN + 1,
    PATROL_RIGHT = PATROL_LEFT + 1,
    PATROL_UP = PATROL_RIGHT + 1,
    TOP_FLOOR = PATROL_UP + 1,
    MID_FLOOR = TOP_FLOOR + 1,
    TOP_WALL = MID_FLOOR + 1,
    SIDE_WALL_MID = TOP_WALL + 1,
    SIDE_WALL_BOT = SIDE_WALL_MID + 1,
    SIDE_WALL_TOP = SIDE_WALL_BOT + 1,
    BOT_WALL = SIDE_WALL_TOP + 1,
    JAIL_DOOR = BOT_WALL + 1,
    JAIL_WALL = JAIL_DOOR + 1,
    WALL_HOLE = JAIL_WALL + 1,
    INNER_WALL = WALL_HOLE + 1,
    PILLOW_BLUE = INNER_WALL + 1,
    PILLOW_BROWN = PILLOW_BLUE + 1,
    PILLOW_GREY = PILLOW_BROWN + 1,
    TUNNEL = PILLOW_GREY + 1,
    TUNNEL_WALL = TUNNEL + 1,
    TUNNEL_FLOOR_1 = TUNNEL_WALL + 1,
    TUNNEL_FLOOR_2 = TUNNEL_FLOOR_1 + 1,
    TUNNEL_FLOOR_3 = TUNNEL_FLOOR_2 + 1,
    CAT_LYING = TUNNEL_FLOOR_3 + 1,
    HOSPITAL_BACK_WALL = CAT_LYING + 1,
    HOSPITAL_FLOOR_TOP = HOSPITAL_BACK_WALL + 1,
    HOSPITAL_FLOOR = HOSPITAL_FLOOR_TOP + 1,
    LOBBY_TILE_MAP = HOSPITAL_FLOOR + 1,
    LOBBY_BEHIND_ASSETS = LOBBY_TILE_MAP + 1,
    SPACE_BACKGROUND = LOBBY_BEHIND_ASSETS + 1,
    START_BUTTON = SPACE_BACKGROUND + 1,
    SUSHI = START_BUTTON + 1,
    KEY = SUSHI + 1,
    GEM = KEY + 1,
    LIGHT_CONE = GEM + 1,
    ROTATING_EARTH = LIGHT_CONE + 1,
    GAME_LOGO = ROTATING_EARTH + 1,
    HP_BAR =  GAME_LOGO + 1,
    TEMP_NPC = HP_BAR + 1,
    HEALTH_BAR = TEMP_NPC + 1,
    EMOTION_BAR = HEALTH_BAR + 1,
    EMOTION_BAR_TICKER = EMOTION_BAR + 1,
    TEXT_BOX = EMOTION_BAR_TICKER + 1,
    ACTION_BUTTON = TEXT_BOX + 1,
    BOUNCER_LEFT = ACTION_BUTTON + 1,
    MONEY = BOUNCER_LEFT + 1,
    SHRIMP = MONEY + 1,
    SET_MOTION_LEFT = SHRIMP + 1,
    SET_MOTION_RIGHT = SET_MOTION_LEFT + 1,
    CITY_MAP = SET_MOTION_RIGHT + 1,
    OFFICE_ROOM = CITY_MAP + 1,
    VET_ROOM = OFFICE_ROOM + 1,
    BROWN_BAG = VET_ROOM + 1,
    BLACK_CAT_PORTRAIT = BROWN_BAG + 1,
    ITEM_BOX = BLACK_CAT_PORTRAIT + 1,
    ATTACK_BOX = ITEM_BOX + 1,
    CAT_ATTACK = ATTACK_BOX + 1,
    NPC_PROJECTILE = CAT_ATTACK + 1,
    HEALER = NPC_PROJECTILE + 1,
    MEDICAL_KIT = HEALER + 1,
    FLASH_LIGHT = MEDICAL_KIT + 1,
    DECKARD = FLASH_LIGHT + 1,
    DORIO = DECKARD + 1,
    RACCOON = DORIO + 1,
    BLUE_FISH = RACCOON + 1,
    SHOW_STATS_BUTTON = BLUE_FISH + 1,
    HIDE_STATS_BUTTON = SHOW_STATS_BUTTON + 1,
    STATS_BOX = HIDE_STATS_BUTTON + 1,
    ORANGE_CAT = STATS_BOX + 1,
    GRAY_CAT = ORANGE_CAT + 1,
    SHELTER_DOG = GRAY_CAT + 1,
    BEIGE_CAT = SHELTER_DOG + 1,
    MUSHROOM = BEIGE_CAT + 1,
    MUSHROOM2 = MUSHROOM + 1,
    MUSHROOM3 = MUSHROOM2 + 1,
    GOLD_NECKLACE = MUSHROOM3 + 1,
    RACOON_ATTACK = GOLD_NECKLACE + 1,
    ENCOUNTER_WALL = RACOON_ATTACK + 1,
    BOUNCE_PROJECTILE = ENCOUNTER_WALL + 1,
    E_PRESS = BOUNCE_PROJECTILE + 1,
    STRONG_NPC_IDLE = E_PRESS + 1,
    MAFIA_ENCOUNTER = STRONG_NPC_IDLE + 1,
    BOSS_ROOM = MAFIA_ENCOUNTER + 1,
    PATROL = BOSS_ROOM + 1,
    BROWN_DOG = PATROL + 1,
    BROWN_DOG_ATTACK = BROWN_DOG + 1,
    WHITE_DOG = BROWN_DOG_ATTACK + 1,
    BROWN_BLACK_DOG = WHITE_DOG + 1,
    WHITE_BROWN_DOG = BROWN_BLACK_DOG + 1,
    BIG_BOSS = WHITE_BROWN_DOG + 1,
    BIG_BOSS_ATTACK = BIG_BOSS + 1,
    DIALOGUE_ICON = BIG_BOSS_ATTACK + 1,
    DIALOGUE_BOX = DIALOGUE_ICON + 1,
    SELECT_ARROW = DIALOGUE_BOX + 1,
    SMOKE_PARTICLE = SELECT_ARROW + 1,
    ENCOUNTER_ICON = SMOKE_PARTICLE + 1,
    STEAK = ENCOUNTER_ICON + 1,
    HEART = STEAK + 1,
    TOMBSTONE = HEART + 1,
    HELP_BACKGROUND = TOMBSTONE + 1,
    TEXTURE_COUNT = HELP_BACKGROUND + 1
};

const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

struct NPC
{
    std::string name;
    int encounter_texture_id;
    bool isInteractable = false;
    bool start_encounter = false;
    bool blocking_door = false;
    bool hasDialogue = false;
    int blocked_door;
    float attackDuration = 0.0f;
    std::queue<std::string> dialogue;
    bool to_remove = false;
    bool isFadingOut = false;
    float fadeOutTimer = 0;
    Entity interactIcon;
    float interactDistance = 110;
    bool isTutorialNPC = false;
    bool isDefeated = false;
    bool dropKey = false;
};

struct EquippableItem
{
    TEXTURE_ASSET_ID texture;
    Stats statModifiers;
    bool isInteractable = false;
    float pickupCooldown = 0.f;
};

enum class EFFECT_ASSET_ID {
    BLACK = 0,
	TEXTURED = BLACK + 1,
    BACKPACK = TEXTURED + 1,
    EFFECT_COUNT = BACKPACK + 1
};

const int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID {
    BACKPACK = 0,
    DEBUG_LINE = BACKPACK + 1,
    SCREEN_TRIANGLE = DEBUG_LINE + 1,
    SPRITE = SCREEN_TRIANGLE + 1,
    GEOMETRY_COUNT = SPRITE + 1
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

struct RenderRequest {
    TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
    Animation animation;
    bool hasAnimation = false;
    float alpha = 1;
    bool flip_horizontal = false;
};

struct VisualEffect {
    float duration;
    float amount;
    std::string type;
};

struct DeathTimer
{
    float counter_ms = 3000;
};

struct Gesture
{
    std::vector<vec2> points;
};

struct KeyInputState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

struct CameraComponent {
    vec2 targetPosition;
};

struct Time {
    float time = 300.f * 60.f * 6.f;
};

struct Key {
    float fadeTimer = 0;
};

struct KeyInventory {
    std::vector<Entity> keys;
};