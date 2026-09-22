#pragma once

#include <map>

#include "core/ecs.hpp"
#include "core/ecs_registry.hpp"
#include "../common.hpp"

class PathFindingSystem {
public:
    PathFindingSystem();
    ~PathFindingSystem();
    bool isActive();
    // void init();
    // void restart();
    void restart_grid(std::vector<std::vector<Node>>& grid);

    void step(std::vector<std::vector<Node>>* room_grid, const std::vector<Entity>& room_entities);


    ///////////////////////////////////////
    // FOR ROOM-SPECIFIC GRAPHS

    // cleanup
    static void cleanup_grid(std::vector<std::vector<Node>>& grid);

    // initializing grid + constructing A* map
    void init_grid(const std::string& room_name, std::vector<std::vector<Node>>*& room_grid, const std::vector<Entity>& room_entities, int room_width, int room_height);
    static void initialize_nodes_in_grid(std::vector<std::vector<Node>>& grid, int grid_width, int grid_height);
    void construct_a_star_graph(std::vector<std::vector<Node>>& grid, const std::vector<Entity>& room_entities,
        int grid_width, int grid_height) const;

    // actual path finding
    std::vector<vec2> a_star_search_in_grid(Node* start, Node* target, std::vector<std::vector<Node>>* grid);
    Node * get_node_from_pos_in_grid(vec2 position, std::vector<std::vector<Node>>* grid, int grid_width, int grid_height) const;
    std::vector<vec2> reconstruct_path_in_grid(Node* targetNode, Node* startNode) const;

    // printing
    static void print_a_star_grid(std::vector<std::vector<Node>>& grid, int grid_width, int grid_height);
private:
    std::map<std::string, std::vector<std::vector<Node>>> grid_map;
    std::vector<std::vector<Node>>* current_grid;
    // void cleanup();
    // void initializeNodes();
    // void constructAStarGraph();
    // void printAStarGraph();
    // std::vector<std::vector<Node>> copyAStarGraph() const;
    // bool areGridsEqual(const std::vector<std::vector<Node>>& grid1, const std::vector<std::vector<Node>>& grid2);
    // Node * getNodeFromPos(vec2 position);
    // std::vector<vec2> AStarSearch(Node* start, Node* target);
    // std::vector<vec2> reconstructPath(Node* targetNode, Node* startNode);
    // void printPath(Node* target, Node* start);

    float cell_size = 5;
    bool is_active = false;
    bool chase_engaged = false;
    bool chase_disengaged = false;

    void print_path_for_grid(std::vector<std::vector<Node>>& grid, int grid_width, int grid_height, Node* target, Node* start);
};