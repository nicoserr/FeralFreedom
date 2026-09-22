#include "systems/pathfinding_system.hpp"

#include <queue>
#include <unordered_set>

// TODO: Make movement smoother.

PathFindingSystem::PathFindingSystem() {
}

PathFindingSystem::~PathFindingSystem() {
    // cleanup();
}

bool PathFindingSystem::isActive() {
    return is_active;
}

// Old code: restart and cleanup
/*
void PathFindingSystem::restart() {
    cleanup();
    init();
}

void PathFindingSystem::cleanup() {
    // Reset all nodes in the grid
    for (auto& row : grid) {
        for (auto& node : row) {
            node.neighbors.clear(); // Clear neighbors to remove dangling references
            node.gCost = 0;
            node.hCost = 0;
            node.fCost = 0;
            node.parent = nullptr;
        }
    }

    // Clear the grid entirely
    grid.clear();
    grid.shrink_to_fit();

    // Reset system state
    is_active = false;
    chase_engaged = false;
    chase_disengaged = false;
}
*/

void PathFindingSystem::cleanup_grid(std::vector<std::vector<Node>>& grid) {
    // Reset all nodes in the grid
    for (auto& row : grid) {
        for (auto& node : row) {
            node.neighbors.clear(); // Clear neighbors to remove dangling references
            node.gCost = 0;
            node.hCost = 0;
            node.fCost = 0;
            node.parent = nullptr;
        }
    }

    // Clear the grid entirely
    grid.clear();
    grid.shrink_to_fit();
}

void PathFindingSystem::step(std::vector<std::vector<Node>>* room_grid, const std::vector<Entity>& room_entities) {
    // Don't pathfind if no convergers.
    if (registry.convergers.components.empty() || registry.chasers.components.empty()) {
        return;
    }

    // Get A* path for all convergers
    for (Entity e : room_entities)
    {
        // Only do this for convergers
        if (!registry.convergers.has(e))
            continue;

        Converger& converger = registry.convergers.get(e);

        if (!converger.path_found) { // Avoid recomputing path
            const auto& grid_ref = *room_grid;
            int grid_width = grid_ref.size();
            int grid_height = (grid_width > 0) ? grid_ref[0].size() : 0;

            // Check if grid is valid.
            if (grid_width == 0 || grid_height == 0) {
                std::cerr << "Error: The grid is empty or improperly initialized." << std::endl;
            }
            chase_engaged = true;
            Motion& converger_motion = registry.motions.get(e);

            Node * converger_node = get_node_from_pos_in_grid(converger_motion.position, room_grid, grid_width, grid_height);
            Node * player_node = get_node_from_pos_in_grid(converger.target_pos, room_grid, grid_width, grid_height);

            converger.path_to_target = a_star_search_in_grid(converger_node, player_node, room_grid);
            converger.path_found = true;
        }
    }

    if (chase_engaged && registry.convergers.entities.size() == 0) {
        chase_engaged = false;
        chase_disengaged = true;
    }
}

Node * PathFindingSystem::get_node_from_pos_in_grid(vec2 position, std::vector<std::vector<Node>>* grid,
                                                    int grid_width, int grid_height) const {
    // Calculate the grid indices
    int xIndex = static_cast<int>(position.x / cell_size);
    int yIndex = static_cast<int>(position.y / cell_size);

    if (xIndex >= 0 && xIndex < grid_width && yIndex >= 0 && yIndex < grid_height) {
        return &(grid->at(xIndex).at(yIndex));
    }

    return nullptr;
}

std::vector<vec2> PathFindingSystem::a_star_search_in_grid(Node* start, Node* target, std::vector<std::vector<Node>>* grid) {
    // Open and closed lists
    std::priority_queue<Node*, std::vector<Node*>, NodeComparator> openList;
    std::unordered_set<Node*> openListSet;
    std::unordered_set<Node*> closedList;
    std::unordered_set<Node*> modifiedNodes;

    // Initialize the start node
    start->gCost = 0;
    start->hCost = start->heuristic(target);
    start->fCost = start->gCost + start->hCost;
    openList.push(start);
    openListSet.insert(start);
    modifiedNodes.insert(start);

    // A* search loop
    while (!openList.empty()) {
        // Get the node with the lowest fCost
        Node* current = openList.top();
        openList.pop();

        // Target reached
        if (current == target) {
            // printPath(current, start);
            std::vector<vec2> tmp = reconstruct_path_in_grid(current, start);
            for (Node* node : modifiedNodes) {
                node->gCost = 0;
                node->hCost = 0;
                node->fCost = 0;
                node->parent = nullptr;
            }
            return tmp;
        }

        closedList.insert(current);

        for (Node* neighbor : current->neighbors) {
            if (neighbor->isBlocked || closedList.find(neighbor) != closedList.end()) {
                continue;
            }

            // Calculate tentative gCost
            float tentativeGCost = current->gCost + 1; // Assumes uniform cost

            if (tentativeGCost < neighbor->gCost || openListSet.find(neighbor) == openListSet.end()) {
                // Update costs
                neighbor->gCost = tentativeGCost;
                neighbor->hCost = neighbor->heuristic(target);
                neighbor->fCost = neighbor->gCost + neighbor->hCost;
                neighbor->parent = current;

                modifiedNodes.insert(neighbor);

                // Add neighbor to the open list
                if (openListSet.find(neighbor) == openListSet.end()) {
                    openList.push(neighbor);
                    openListSet.insert(neighbor);
                }
            }
        }
    }

    for (Node* node : modifiedNodes) {
        node->gCost = 0;
        node->hCost = 0;
        node->fCost = 0;
        node->parent = nullptr;
    }
    return {};
}

std::vector<vec2> PathFindingSystem::reconstruct_path_in_grid(Node* targetNode, Node* startNode) const {
    std::vector<vec2> pathPositions;

    Node* currentNode = targetNode;
    while (currentNode != startNode) {
        pathPositions.emplace_back(currentNode->x * cell_size, currentNode->y * cell_size);
        currentNode = currentNode->parent;
    }

    // Add the start node position as well
    pathPositions.emplace_back(startNode->x * cell_size, startNode->y * cell_size);

    // Reverse the path to start from the startNode
    std::reverse(pathPositions.begin(), pathPositions.end());

    return pathPositions;
}

// Old Code:
/*
std::vector<vec2> PathFindingSystem::reconstructPath(Node *targetNode, Node *startNode) {
    std::vector<vec2> pathPositions;

    Node* currentNode = targetNode;
    while (currentNode != startNode) {
        pathPositions.emplace_back(currentNode->x * cell_size, currentNode->y * cell_size);
        currentNode = currentNode->parent;
    }

    // Add the start node position as well
    pathPositions.emplace_back(startNode->x * cell_size, startNode->y * cell_size);

    // Reverse the path to start from the startNode
    std::reverse(pathPositions.begin(), pathPositions.end());

    return pathPositions;
}

Node * PathFindingSystem::getNodeFromPos(vec2 position) {
    // Calculate the grid indices
    int xIndex = static_cast<int>(position.x / cell_size);
    int yIndex = static_cast<int>(position.y / cell_size);

    if (xIndex >= 0 && xIndex < grid_width && yIndex >= 0 && yIndex < grid_height) {
        return &grid[xIndex][yIndex];
    }

    return nullptr;
}

std::vector<vec2> PathFindingSystem::AStarSearch(Node* start, Node* target) {
    // Open and closed lists
    std::priority_queue<Node*, std::vector<Node*>, NodeComparator> openList;
    std::unordered_set<Node*> openListSet;
    std::unordered_set<Node*> closedList;
    std::unordered_set<Node*> modifiedNodes;

    // Initialize the start node
    start->gCost = 0;
    start->hCost = start->heuristic(target);
    start->fCost = start->gCost + start->hCost;
    openList.push(start);
    openListSet.insert(start);
    modifiedNodes.insert(start);

    // A* search loop
    while (!openList.empty()) {
        // Get the node with the lowest fCost
        Node* current = openList.top();
        openList.pop();

        // Target reached
        if (current == target) {
            // printPath(current, start);
            std::vector<vec2> tmp = reconstructPath(current, start);
            for (Node* node : modifiedNodes) {
                node->gCost = 0;
                node->hCost = 0;
                node->fCost = 0;
                node->parent = nullptr;
            }
            return tmp;
        }

        closedList.insert(current);

        for (Node* neighbor : current->neighbors) {
            if (neighbor->isBlocked || closedList.find(neighbor) != closedList.end()) {
                continue;
            }

            // Calculate tentative gCost
            float tentativeGCost = current->gCost + 1; // Assumes uniform cost

            if (tentativeGCost < neighbor->gCost || openListSet.find(neighbor) == openListSet.end()) {
                // Update costs
                neighbor->gCost = tentativeGCost;
                neighbor->hCost = neighbor->heuristic(target);
                neighbor->fCost = neighbor->gCost + neighbor->hCost;
                neighbor->parent = current;

                modifiedNodes.insert(neighbor);

                // Add neighbor to the open list
                if (openListSet.find(neighbor) == openListSet.end()) {
                    openList.push(neighbor);
                    openListSet.insert(neighbor);
                }
            }
        }
    }

    for (Node* node : modifiedNodes) {
        node->gCost = 0;
        node->hCost = 0;
        node->fCost = 0;
        node->parent = nullptr;
    }
    return {};
}
*/

void PathFindingSystem::print_path_for_grid(std::vector<std::vector<Node>>& grid, int grid_width, int grid_height, Node* target, Node* start) {
    // Create a 2D array to represent the path on the grid
    std::vector<std::vector<char>> pathGrid(grid_width, std::vector<char>(grid_height, '.'));

    // Traverse from the target node back to the start node
    Node* current = target;
    while (current != start) {
        // Mark the path node on the grid
        pathGrid[current->x][current->y] = '*'; // Using '*' to represent the path
        current = current->parent; // Move to the parent node
    }

    // Mark the start node with 'S' and the target node with 'T'
    pathGrid[start->x][start->y] = 'S'; // Start node
    pathGrid[target->x][target->y] = 'T'; // Target node

    // Print the path grid
    for (int y = 0; y < grid_height; ++y) {
        for (int x = 0; x < grid_width; ++x) {

           if (grid[x][y].isBlocked)
               pathGrid[x][y] = 'X';
        }
    }
}