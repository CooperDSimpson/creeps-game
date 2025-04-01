#pragma once
#include <iostream>
#include <cmath>
#include <string>
#include <Windows.h>
#include "item.hpp"
#include "tile.hpp"
#include "colors.hpp"

class Entity {

public:
    //components

    bool is_ghost = false;
    bool is_enemy = true;

    //end components
    double delta_x = 0;
    double delta_y = 0;
    double true_x = 0;
    double true_y = 0;
    double x_speed = 0;
    double y_speed = 0;
    double MAX_SPEED = 10;
    double FRICTION_DECELERATION = 120;
    unsigned int health = 100;
    std::vector<Item> inventory;
    wchar_t entity_graphic = L'?';
    int entity_graphic_color = FG_WHITE;
    std::wstring name = L"unnamed";      // Unicode
    int y = 0;
    int x = 0;

    void set_position(int new_x, int new_y) {
        x = new_x;
        y = new_y;
        true_x = x;
        true_y = y;
    }
    



    bool isValidPosition(double test_x, double test_y, World& world) {
        // If entity is a ghost, any position is valid
        if (is_ghost) return true;

        int grid_x = static_cast<int>(test_x);
        int grid_y = static_cast<int>(test_y);

        // Check world boundaries
        if (grid_x < 0 || grid_x >= world.world_width || grid_y < 0 || grid_y >= world.world_height)
            return false;

        // Check if the tile is solid
        if (is_enemy && !world.grid[grid_y][grid_x].enemy_passthrough) {
            return 0;
        }
        else if(!is_enemy && !world.grid[grid_y][grid_x].player_passthrough) {
            return 0;
        }
        return !world.grid[grid_y][grid_x].solid;
    }

    void move(int world_y_size, int world_x_size, double delta_time, World& world) {
        // Apply friction-based deceleration
        double friction_factor = FRICTION_DECELERATION * delta_time;

        // Apply friction only if the entity is moving
        if (std::abs(x_speed) > 0) {
            x_speed -= (x_speed > 0) ? friction_factor : -friction_factor;
            if (std::abs(x_speed) < friction_factor) {
                x_speed = 0;
            }
        }

        if (std::abs(y_speed) > 0) {
            y_speed -= (y_speed > 0) ? friction_factor : -friction_factor;
            if (std::abs(y_speed) < friction_factor) {
                y_speed = 0;
            }
        }

        // Cap individual speed components
        x_speed = std::clamp(x_speed, -MAX_SPEED, MAX_SPEED);
        y_speed = std::clamp(y_speed, -MAX_SPEED, MAX_SPEED);

        // Get current integer position
        int current_grid_x = static_cast<int>(true_x);
        int current_grid_y = static_cast<int>(true_y);

        // Calculate movement deltas
        double dx = x_speed * delta_time;
        double dy = y_speed * delta_time;

        // Separate X and Y movements and test them in small increments

        // First, try X-axis movement
        double test_x = true_x;
        const int X_STEPS = 20; // More detailed steps
        double x_step = dx / X_STEPS;

        for (int i = 1; i <= X_STEPS; i++) {
            double next_x = true_x + x_step * i;
            // Check both the top and bottom edges of the entity's cell for collision
            if (!isValidPosition(next_x, true_y, world)) {
                // Hit a wall in X direction
                x_speed = 0;
                break;
            }
            test_x = next_x;
        }

        // Then, try Y-axis movement
        double test_y = true_y;
        const int Y_STEPS = 20;
        double y_step = dy / Y_STEPS;

        for (int i = 1; i <= Y_STEPS; i++) {
            double next_y = true_y + y_step * i;
            // Check both the left and right edges of the entity's cell for collision
            if (!isValidPosition(test_x, next_y, world)) {
                // Hit a wall in Y direction
                y_speed = 0;
                break;
            }
            test_y = next_y;
        }

        // Ensure we're not crossing grid cell boundaries if there's a diagonal wall
        int new_grid_x = static_cast<int>(test_x);
        int new_grid_y = static_cast<int>(test_y);

        // If we're trying to move diagonally across grid cells
        if (new_grid_x != current_grid_x && new_grid_y != current_grid_y) {
            // Check if the diagonal movement is valid
            if (!isValidPosition(new_grid_x, new_grid_y, world) ||
                !isValidPosition(new_grid_x, current_grid_y, world) ||
                !isValidPosition(current_grid_x, new_grid_y, world)) {
                // If any corner is blocked, prevent diagonal movement
                if (!isValidPosition(new_grid_x, current_grid_y, world)) {
                    test_x = current_grid_x;
                    x_speed = 0;
                }
                if (!isValidPosition(current_grid_x, new_grid_y, world)) {
                    test_y = current_grid_y;
                    y_speed = 0;
                }
            }
        }

        // Apply the final movement
        true_x = test_x;
        true_y = test_y;

        // Update the entity's grid position (integer values)
        x = static_cast<int>(true_x);
        y = static_cast<int>(true_y);

        // Add a buffer zone near walls
        const double WALL_BUFFER = 0.05; // A small buffer to keep entity away from walls

        // Check if we're too close to walls and adjust position if needed
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int check_x = x + dx;
                int check_y = y + dy;

                // Skip checking our own cell
                if (dx == 0 && dy == 0) continue;

                // If there's a wall nearby
                if (check_x >= 0 && check_x < world_x_size &&
                    check_y >= 0 && check_y < world_y_size &&
                    !isValidPosition(check_x, check_y, world)) {

                    // Calculate distance to the wall and adjust if too close
                    double dist_x = std::abs(true_x - check_x);
                    double dist_y = std::abs(true_y - check_y);

                    if (dist_x < WALL_BUFFER && dx != 0) {
                        true_x += (dx < 0) ? WALL_BUFFER : -WALL_BUFFER;
                    }
                    if (dist_y < WALL_BUFFER && dy != 0) {
                        true_y += (dy < 0) ? WALL_BUFFER : -WALL_BUFFER;
                    }
                }
            }
        }

        // Final position update after all adjustments
        x = static_cast<int>(true_x);
        y = static_cast<int>(true_y);
    }




    void add_item_to_inventory(Item item) {
        bool found = false;
        for (Item& item_ : inventory) {
            if (item_.name == item.name) {
                item_.amount += 1;
                found = true;
            }
        }
        if (!found) {
            inventory.push_back(item);
        }
    }

    void attack(Entity& target, int damage) {
        target.health -= damage;
    }

    void damage(int damage_amount) {
        health -= damage_amount;
    }
};
