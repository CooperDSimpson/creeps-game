#pragma once
#include "entity.hpp"
#include "player.hpp"

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <sstream>
#include <Windows.h>
#include <random>
#include "colors.hpp"
#include <queue>
#include <unordered_map>
#include <cmath>

struct Node {
    int x, y;
    float cost, heuristic;
    Node* parent;

    Node(int x, int y, float cost, float heuristic, Node* parent = nullptr)
        : x(x), y(y), cost(cost), heuristic(heuristic), parent(parent) {
    }

    float total_cost() const { return cost + heuristic; }
};

struct CompareNodes {
    bool operator()(const Node* a, const Node* b) {
        return a->total_cost() > b->total_cost();
    }
};

class Creep : public Entity {
public:
    Creep() {
        entity_graphic = 'X';
        entity_graphic_color = FG_RED;
        name = L"creep";
        FRICTION_DECELERATION = 120;
        MAX_SPEED = 20;
        y = 0;
        x = 0;
        wanderCounter = 0;
        currentDirection = -1;
    }

    int wanderCounter;
    int currentDirection;
    int target_x;
    int target_y;
    const static int world_x_size = 200;
    const static int world_y_size = 200;
    bool do_wander = true;

    std::vector<std::pair<int, int>> reconstruct_path(Node* node) {
        std::vector<std::pair<int, int>> path;
        while (node) {
            path.emplace_back(node->x, node->y);
            node = node->parent;
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    bool is_passable(int x, int y, World& world_map) {
        // Check if the tile is valid for an enemy
        if (!world_map.grid[y][x].enemy_passthrough) {
            return false;
        }
        return !world_map.grid[y][x].solid;
    }

    std::vector<std::pair<int, int>> a_star(int start_x, int start_y, int goal_x, int goal_y, World& world_map) {
        std::priority_queue<Node*, std::vector<Node*>, CompareNodes> open_list;
        std::unordered_map<int, Node*> all_nodes;
        auto key = [](int x, int y) { return y * world_x_size + x; };

        Node* start = new Node(start_x, start_y, 0, std::hypot(goal_x - start_x, goal_y - start_y));
        open_list.push(start);
        all_nodes[key(start_x, start_y)] = start;

        std::vector<std::pair<int, int>> directions = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };

        const int MAX_ITERATIONS = 10000;  // Prevent infinite loops
        int iterations = 0;

        while (!open_list.empty()) {
            if (++iterations > MAX_ITERATIONS) {
                //wander(world_map);
                return {};  // Bail out to prevent lag
            }

            Node* current = open_list.top();
            open_list.pop();

            if (current->x == goal_x && current->y == goal_y) {
                return reconstruct_path(current);
            }

            for (auto [dx, dy] : directions) {
                int nx = current->x + dx;
                int ny = current->y + dy;
                if (nx < 0 || ny < 0 || nx >= world_x_size || ny >= world_y_size || !is_passable(nx, ny, world_map)) continue;

                float new_cost = current->cost + 1;
                int node_key = key(nx, ny);

                if (!all_nodes.count(node_key) || new_cost < all_nodes[node_key]->cost) {
                    Node* neighbor = new Node(nx, ny, new_cost, std::hypot(goal_x - nx, goal_y - ny), current);
                    open_list.push(neighbor);
                    all_nodes[node_key] = neighbor;
                }
            }
        }

        //wander(world_map);
        return {};  // Return empty if no path exists
    }

    void wander(World& world_map) {
        srand(time(0));
        target_x = rand() % world_x_size;
        target_y = rand() % world_y_size;
        auto path = a_star(x, y, target_x, target_y, world_map);
        if (path.size() > 1) {
            // Calculate direction to next path point
            int next_x = path[1].first;
            int next_y = path[1].second;

            // Set velocity based on direction
            if (next_x > x) x_speed = MAX_SPEED;
            else if (next_x < x) x_speed = -MAX_SPEED;

            if (next_y > y) y_speed = MAX_SPEED;
            else if (next_y < y) y_speed = -MAX_SPEED;
        }
    }

    void pursue_position(int pos_x, int pos_y, World& world_map) {
        if (do_wander) {
            wander(world_map);
            return;
        };

        target_x = pos_x;
        target_y = pos_y;

        auto path = a_star(x, y, target_x, target_y, world_map);
        if (path.size() > 1) {
            // Calculate direction to next path point
            int next_x = path[1].first;
            int next_y = path[1].second;

            // Set velocity based on direction
            if (next_x > x) x_speed = MAX_SPEED;
            else if (next_x < x) x_speed = -MAX_SPEED;

            if (next_y > y) y_speed = MAX_SPEED;
            else if (next_y < y) y_speed = -MAX_SPEED;
        }
    }
};
