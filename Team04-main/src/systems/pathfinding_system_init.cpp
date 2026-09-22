#include "pathfinding_system.hpp"


// void PathFindingSystem::init() {
//     printf("Initializing Pathfinding System...\n");
// }

void PathFindingSystem::init_grid(const std::string& room_name, std::vector<std::vector<Node>>*& room_grid, const std::vector<Entity>& room_entities, int room_width, int room_height) {
    auto it_map = grid_map.find(room_name);

    if (it_map == grid_map.end())
    {
        // Doesn't exist, initialize
        std::vector<std::vector<Node>> grid;
        int grid_width = room_width/cell_size;
        int grid_height = room_height/cell_size;
        grid.resize(grid_width, std::vector<Node>(grid_height));
        grid_map.emplace(room_name, std::vector<std::vector<Node>>(grid));
        it_map = grid_map.find(room_name);

        initialize_nodes_in_grid(it_map->second, grid_width, grid_height);
        construct_a_star_graph(it_map->second, room_entities, grid_width, grid_height);

        // grid_map.emplace(room_name, std::vector<std::vector<Node>>(grid));
        // it_map = grid_map.find(room_name);
    }

    room_grid = &it_map->second;
}

void PathFindingSystem::initialize_nodes_in_grid(std::vector<std::vector<Node>>& grid, int grid_width, int grid_height) {
    for (int x = 0; x < grid_width; ++x) {
        for (int y = 0; y < grid_height; ++y) {
            grid[x][y] = Node(x, y);
        }
    }
}

void PathFindingSystem::construct_a_star_graph(std::vector<std::vector<Node>>& grid, const std::vector<Entity>& room_entities,
    int grid_width, int grid_height) const {
    // Mark obstacles in the grid
    // printf("Constructing A* Graph...\n");
    for (Entity e : room_entities) {
        // printf("Processing Entity ID: %u\n", (unsigned int)e);

        if (!registry.colliders.has(e))
        {
            continue;
        }

        Collider& collider = registry.colliders.get(e);
        if (collider.type == OBSTACLE || collider.type == DOOR) {
            // Calculate the grid cells that this obstacle occupies
            BoundingBox& bounding_box = registry.boundingBoxes.get(e);
            Motion& motion = registry.motions.get(e);

            int x_start = static_cast<int>((motion.position.x - bounding_box.width/2)  / cell_size);
            int y_start = static_cast<int>((motion.position.y - bounding_box.height/2)  / cell_size);
            int x_end = static_cast<int>((motion.position.x + bounding_box.width/2) / cell_size);
            int y_end = static_cast<int>((motion.position.y + bounding_box.height/2)  / cell_size);

            // Mark the obstacle nodes as blocked
            for (int x = x_start; x <= x_end; ++x) {
                for (int y = y_start; y <= y_end; ++y) {
                    if (x >= 0 && x < grid_width && y >= 0 && y < grid_height) { // Check bounds
                        grid[x][y].isBlocked = true;
                    }
                }
            }
        }
    }
    // Connect neighboring nodes in the grid (8-way)
    for (int x = 0; x < grid_width; ++x) {
        for (int y = 0; y < grid_height; ++y) {
            if (!grid[x][y].isBlocked) {
                // W
                if (x > 0 && !grid[x - 1][y].isBlocked) grid[x][y].neighbors.push_back(&grid[x - 1][y]);
                // E
                if (x < grid_width - 1 && !grid[x + 1][y].isBlocked) grid[x][y].neighbors.push_back(&grid[x + 1][y]);
                // S
                if (y > 0 && !grid[x][y - 1].isBlocked) grid[x][y].neighbors.push_back(&grid[x][y - 1]);
                // N
                if (y < grid_height - 1 && !grid[x][y + 1].isBlocked) grid[x][y].neighbors.push_back(&grid[x][y + 1]);
                // NW
                if (x > 0 && y > 0 && !grid[x - 1][y - 1].isBlocked) grid[x][y].neighbors.push_back(&grid[x - 1][y - 1]);
                // NE
                if (x < grid_width - 1 && y > 0 && !grid[x + 1][y - 1].isBlocked) grid[x][y].neighbors.push_back(&grid[x + 1][y - 1]);
                // SW
                if (x > 0 && y < grid_height - 1 && !grid[x - 1][y + 1].isBlocked) grid[x][y].neighbors.push_back(&grid[x - 1][y + 1]);
                // SE
                if (x < grid_width - 1 && y < grid_height - 1 && !grid[x + 1][y + 1].isBlocked) grid[x][y].neighbors.push_back(&grid[x + 1][y + 1]);
            }
        }
    }
    // printf("A* Graph construction complete.\n");
}

void PathFindingSystem::print_a_star_grid(std::vector<std::vector<Node>>& grid, int grid_width, int grid_height) {
    // printf("Printing PathFinding Graph: \n");
    for (int y = 0; y < grid_height; ++y) {
        for (int x = 0; x < grid_width; ++x) {
            std::cout << (grid[x][y].isBlocked ? 'X' : '.');
        }
        std::cout << std::endl;
    }
}

// Old code:
/*
void PathFindingSystem::initializeNodes() {
    for (int x = 0; x < grid_width; ++x) {
        for (int y = 0; y < grid_height; ++y) {
            grid[x][y] = Node(x, y);
        }
    }
}

void PathFindingSystem::constructAStarGraph() {
    // Mark obstacles in the grid
    // printf("Constructing A* Graph...\n");
    for (Entity e : registry.colliders.entities) {
        // printf("Processing Entity ID: %u\n", (unsigned int)e);

        Collider& collider = registry.colliders.get(e);
        if (collider.type == OBSTACLE) {
            // Calculate the grid cells that this obstacle occupies
            BoundingBox& bounding_box = registry.boundingBoxes.get(e);
            Motion& motion = registry.motions.get(e);

            int x_start = static_cast<int>((motion.position.x - bounding_box.width/2)  / cell_size);
            int y_start = static_cast<int>((motion.position.y - bounding_box.height/2)  / cell_size);
            int x_end = static_cast<int>((motion.position.x + bounding_box.width/2) / cell_size);
            int y_end = static_cast<int>((motion.position.y + bounding_box.height/2)  / cell_size);

            // Mark the obstacle nodes as blocked
            for (int x = x_start; x <= x_end; ++x) {
                for (int y = y_start; y <= y_end; ++y) {
                    if (x >= 0 && x < grid_width && y >= 0 && y < grid_height) { // Check bounds
                        grid[x][y].isBlocked = true;
                    }
                }
            }
        }
    }
    // Connect neighboring nodes in the grid (8-way)
    for (int x = 0; x < grid_width; ++x) {
        for (int y = 0; y < grid_height; ++y) {
            if (!grid[x][y].isBlocked) {
                // W
                if (x > 0 && !grid[x - 1][y].isBlocked) grid[x][y].neighbors.push_back(&grid[x - 1][y]);
                // E
                if (x < grid_width - 1 && !grid[x + 1][y].isBlocked) grid[x][y].neighbors.push_back(&grid[x + 1][y]);
                // S
                if (y > 0 && !grid[x][y - 1].isBlocked) grid[x][y].neighbors.push_back(&grid[x][y - 1]);
                // N
                if (y < grid_height - 1 && !grid[x][y + 1].isBlocked) grid[x][y].neighbors.push_back(&grid[x][y + 1]);
                // NW
                if (x > 0 && y > 0 && !grid[x - 1][y - 1].isBlocked) grid[x][y].neighbors.push_back(&grid[x - 1][y - 1]);
                // NE
                if (x < grid_width - 1 && y > 0 && !grid[x + 1][y - 1].isBlocked) grid[x][y].neighbors.push_back(&grid[x + 1][y - 1]);
                // SW
                if (x > 0 && y < grid_height - 1 && !grid[x - 1][y + 1].isBlocked) grid[x][y].neighbors.push_back(&grid[x - 1][y + 1]);
                // SE
                if (x < grid_width - 1 && y < grid_height - 1 && !grid[x + 1][y + 1].isBlocked) grid[x][y].neighbors.push_back(&grid[x + 1][y + 1]);
            }
        }
    }
    // printf("A* Graph construction complete.\n");
}

void PathFindingSystem::printAStarGraph() {
    // printf("Printing PathFinding Graph: \n");
    for (int y = 0; y < grid_height; ++y) {
        for (int x = 0; x < grid_width; ++x) {
            std::cout << (grid[x][y].isBlocked ? 'X' : '.');
        }
        std::cout << std::endl;
    }
}
*/