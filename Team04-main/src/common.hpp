#pragma once

// standard libs
#include <string>
#include <tuple>
#include <vector>

// glfw (OpenGL)
#define NOMINMAX
#include <gl3w.h>
#include <GLFW/glfw3.h>

//SDL
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_mixer.h>


#include <glm/mat3x3.hpp>           // mat3
#include <glm/ext/vector_int2.hpp>  // ivec2
#include <glm/vec2.hpp>	
#include <glm/vec3.hpp>             // vec3
using namespace glm;

#include "core/ecs.hpp"

#include "../ext/project_path.hpp"

inline std::string data_path() { return std::string(PROJECT_SOURCE_DIR) + "data"; };
inline std::string shader_path(const std::string& name) {return std::string(PROJECT_SOURCE_DIR) + "/shaders/" + name;};
inline std::string textures_path(const std::string& name) {return data_path() + "/textures/" + std::string(name);};
inline std::string audio_path(const std::string& name) {return data_path() + "/audio/" + std::string(name);};
inline std::string save_path() {return data_path() + "/json/PlayState.json";};
inline std::string save_path_dir() {return data_path() + "/json/";};
inline std::string stat_to_string(const float stat) {return std::to_string(static_cast<int>(std::floor(stat)));};

extern int window_width_px;
extern int window_height_px;
extern float defaultTextScale;

void getWindowSize();

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

struct Transform {
	mat3 mat = { { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f}, { 0.f, 0.f, 1.f} }; // start with the identity
	void scale(vec2 scale);
	void rotate(float radians);
	void translate(vec2 offset);
};

bool gl_has_errors();
extern bool game_over;

struct Node {
	bool isBlocked;            // Indicates if the node is blocked by an obstacle
	std::vector<Node*> neighbors; // List of neighboring nodes
	int x, y;                 // Position of the node in the grid
	float gCost;              // Cost from start node to this node
	float hCost;              // Heuristic cost to the target node
	float fCost;              // Total cost (gCost + hCost)
	Node* parent;

	Node() // Default constructor
		: isBlocked(false), x(0), y(0), gCost(0), hCost(0), fCost(0)  {}

	Node(int xPos, int yPos) // Parameterized constructor
		: isBlocked(false), x(xPos), y(yPos), gCost(0), hCost(0), fCost(0) {}

	// Optional: Method to calculate the heuristic distance (using Euclidean distance)
	float heuristic(Node* target) const {
		return (x - target->x) * (x - target->x) + (y - target->y) * (y - target->y);
	}
};

struct NodeComparator {
	bool operator()(const Node* node1, const Node* node2) const {
		return node1->fCost > node2->fCost; // Higher fCost should have lower priority
	}
};

mat3 createShadowProjectionMatrix();