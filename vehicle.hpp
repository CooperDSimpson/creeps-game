#pragma once
#include "entity.hpp"
#include "world.hpp"
#include <vector>
#include <memory>
#include <string>
#include <atomic>
#include <stdio.h>
#include <Windows.h>
#include <thread>
#include <iostream>
#include <vector>
#include <utility>
#include <io.h>      
#include <fcntl.h>   
#include <chrono>
#include <thread>
#include <algorithm>
#include <chrono>
#include <unordered_map>

enum class VehiclePartType {
    CHASSIS,
    WHEEL,
    ENGINE,
    SEAT,
    STEERING,
    DECORATION
};

struct VehiclePartConnection {
    int x_offset;
    int y_offset;
    bool connected;
    int connected_to_part_id;
};

class VehiclePart {
public:
    int id;
    std::wstring name;
    VehiclePartType type;
    wchar_t graphic;
    int graphic_color;
    int durability;
    int weight;
    bool is_anchor; // Can other parts be attached to this?
    std::vector<VehiclePartConnection> connection_points;
    int x_offset = 0; // Offset from the vehicle origin
    int y_offset = 0;

    // Type-specific properties
    union {
        struct { // Engine
            int power;
            int fuel_consumption;
        };
        struct { // Wheel
            bool powered;
            int grip;
        };
        struct { // Chassis
            int max_connections;
        };
    };

    VehiclePart(int id, std::wstring name, VehiclePartType type, wchar_t graphic, int graphic_color)
        : id(id), name(name), type(type), graphic(graphic), graphic_color(graphic_color),
        durability(100), weight(10), is_anchor(false) {
    }
};

class Vehicle : public Entity {
private:
    std::vector<std::shared_ptr<VehiclePart>> parts;
    int power;
    int max_speed;
    int turning_rate;
    int current_direction; // 0-359 degrees
    int chassis_id; // Reference to main chassis part
    bool engine_running;

public:
    bool in_building_mode;

    Vehicle() {
        entity_graphic = 'V';
        entity_graphic_color = FG_CYAN;
        name = L"vehicle";
        MAX_SPEED = 50;
        FRICTION_DECELERATION = 40;
        is_enemy = false;
        power = 0;
        max_speed = 0;
        turning_rate = 5;
        current_direction = 0;
        engine_running = false;
        in_building_mode = false;
    }

    bool addPart(std::shared_ptr<VehiclePart> part, int x_offset, int y_offset) {
        // Check if valid placement
        for (const auto& existing_part : parts) {
            if (existing_part->x_offset == x_offset && existing_part->y_offset == y_offset) {
                return false; // Collision with existing part
            }
        }

        part->x_offset = x_offset;
        part->y_offset = y_offset;
        parts.push_back(part);

        // Update vehicle properties based on parts
        recalculateProperties();
        return true;
    }

    bool removePart(int part_id) {
        auto it = std::find_if(parts.begin(), parts.end(),
            [part_id](const std::shared_ptr<VehiclePart>& p) {
                return p->id == part_id;
            });

        if (it != parts.end()) {
            parts.erase(it);
            recalculateProperties();
            return true;
        }
        return false;
    }

    void recalculateProperties() {
        // Reset properties
        power = 0;
        max_speed = 0;
        int total_weight = 0;

        // Count parts by type
        int num_engines = 0;
        int num_wheels = 0;

        // Sum up contributions from parts
        for (const auto& part : parts) {
            total_weight += part->weight;

            if (part->type == VehiclePartType::ENGINE) {
                power += part->power;
                num_engines++;
            }
            else if (part->type == VehiclePartType::WHEEL) {
                if (part->powered) {
                    max_speed += part->grip;
                }
                num_wheels++;
            }
        }

        // Apply formulas to determine vehicle capabilities
        if (num_engines > 0 && num_wheels > 0) {
            MAX_SPEED = min(50, (power / total_weight) * 100);
            // Must have minimum of 4 wheels for max stability
            if (num_wheels < 4) {
                MAX_SPEED = MAX_SPEED * (0.5 + (num_wheels * 0.125));
            }
        }
        else {
            MAX_SPEED = 0; // Can't move without engines and wheels
        }

        // Update entity graphic based on vehicle direction
        updateAppearance();
    }

    void updateAppearance() {
        // Change the vehicle appearance based on current_direction
        // This is simplified - in a real implementation, you might have different graphics
        // for different directions or render the vehicle differently
        switch (current_direction / 90) { // Divide by 90 to get 0-3 for four directions
        case 0: entity_graphic = '>'; break; // East
        case 1: entity_graphic = 'v'; break; // South
        case 2: entity_graphic = '<'; break; // West
        case 3: entity_graphic = '^'; break; // North
        }
    }

    void accelerate(double acceleration) {
        if (!engine_running || parts.empty()) return;

        // Apply acceleration in the direction the vehicle is facing
        double rad_angle = current_direction * 0.0174533; // Convert degrees to radians
        x_speed += std::cos(rad_angle) * acceleration;
        y_speed += std::sin(rad_angle) * acceleration;

        // Cap speed
        double current_speed = std::sqrt(x_speed * x_speed + y_speed * y_speed);
        if (current_speed > MAX_SPEED) {
            double scale = MAX_SPEED / current_speed;
            x_speed *= scale;
            y_speed *= scale;
        }
    }

    void turn(int direction) {
        // +1 for right turn, -1 for left turn
        current_direction = (current_direction + direction * turning_rate) % 360;
        if (current_direction < 0) current_direction += 360;

        updateAppearance();
    }

    void toggleEngine() {
        // Check if the vehicle has at least one engine
        bool has_engine = false;
        for (const auto& part : parts) {
            if (part->type == VehiclePartType::ENGINE) {
                has_engine = true;
                break;
            }
        }

        if (has_engine) {
            engine_running = !engine_running;
        }
        else {
            engine_running = false;
        }
    }

    bool isEngineRunning() const {
        return engine_running;
    }

    void render(CHAR_INFO*& screen, int player_x, int player_y, int screen_width, int screen_height) {
        int half_screen_width = screen_width / 2;
        int half_screen_height = screen_height / 2;

        // Display the vehicle frame
        int screen_x = x - player_x + half_screen_width;
        int screen_y = y - player_y + half_screen_height;

        // Check if vehicle is on screen
        if (screen_x >= 0 && screen_x < screen_width && screen_y >= 0 && screen_y < screen_height) {
            screen[screen_y * screen_width + screen_x].Char.UnicodeChar = entity_graphic;
            screen[screen_y * screen_width + screen_x].Attributes = entity_graphic_color;

            // Display all vehicle parts relative to vehicle position
            for (const auto& part : parts) {
                int part_screen_x = screen_x + part->x_offset;
                int part_screen_y = screen_y + part->y_offset;

                if (part_screen_x >= 0 && part_screen_x < screen_width &&
                    part_screen_y >= 0 && part_screen_y < screen_height) {
                    screen[part_screen_y * screen_width + part_screen_x].Char.UnicodeChar = part->graphic;
                    screen[part_screen_y * screen_width + part_screen_x].Attributes = part->graphic_color;
                }
            }
        }
    }

    std::vector<std::shared_ptr<VehiclePart>> getParts() const {
        return parts;
    }

    void enterBuildMode() {
        in_building_mode = true;
        engine_running = false;
    }

    void exitBuildMode() {
        in_building_mode = false;
    }
};

// Pre-defined vehicle part templates
namespace VehicleParts {
    static std::shared_ptr<VehiclePart> createChassis() {
        auto chassis = std::make_shared<VehiclePart>(1, L"Chassis", VehiclePartType::CHASSIS, '#', FG_WHITE);
        chassis->is_anchor = true;
        chassis->weight = 50;
        chassis->max_connections = 8;
        return chassis;
    }

    static std::shared_ptr<VehiclePart> createWheel() {
        auto wheel = std::make_shared<VehiclePart>(2, L"Wheel", VehiclePartType::WHEEL, 'O', FG_GREY);
        wheel->weight = 5;
        wheel->powered = true;
        wheel->grip = 10;
        return wheel;
    }

    static std::shared_ptr<VehiclePart> createEngine() {
        auto engine = std::make_shared<VehiclePart>(3, L"Engine", VehiclePartType::ENGINE, 'E', FG_RED);
        engine->weight = 30;
        engine->power = 100;
        engine->fuel_consumption = 10;
        return engine;
    }

    static std::shared_ptr<VehiclePart> createSeat() {
        auto seat = std::make_shared<VehiclePart>(4, L"Seat", VehiclePartType::SEAT, 'H', FG_BLUE);
        seat->weight = 5;
        return seat;
    }

    static std::shared_ptr<VehiclePart> createSteering() {
        auto steering = std::make_shared<VehiclePart>(5, L"Steering", VehiclePartType::STEERING, '+', FG_YELLOW);
        steering->weight = 5;
        return steering;
    }
}

// Vehicle builder mode UI and logic
class VehicleBuilder {
private:
    Vehicle& vehicle;
    int cursor_x = 0;
    int cursor_y = 0;
    float cursor_true_x = 0;
    float cursor_true_y = 0;

    int selected_part_type = 0;
    std::vector<std::shared_ptr<VehiclePart>> part_templates;

public:
    VehicleBuilder(Vehicle& vehicle) : vehicle(vehicle) {
        // Initialize part templates
        part_templates.push_back(VehicleParts::createChassis());
        part_templates.push_back(VehicleParts::createWheel());
        part_templates.push_back(VehicleParts::createEngine());
        part_templates.push_back(VehicleParts::createSeat());
        part_templates.push_back(VehicleParts::createSteering());
    }

    void handleInput() {
        // Move cursor
        if (GetAsyncKeyState(VK_UP) & 0x8000) cursor_true_y -= 0.05;
        if (GetAsyncKeyState(VK_DOWN) & 0x8000) cursor_true_y += 0.05;
        if (GetAsyncKeyState(VK_LEFT) & 0x8000) cursor_true_x -= 0.05;
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) cursor_true_x += 0.05;
 
        cursor_x = static_cast<int>(cursor_true_x);
        cursor_y = static_cast<int>(cursor_true_y);

        // Change selected part type
        if (GetAsyncKeyState('1') & 0x8000) selected_part_type = 0; // Chassis
        if (GetAsyncKeyState('2') & 0x8000) selected_part_type = 1; // Wheel
        if (GetAsyncKeyState('3') & 0x8000) selected_part_type = 2; // Engine
        if (GetAsyncKeyState('4') & 0x8000) selected_part_type = 3; // Seat
        if (GetAsyncKeyState('5') & 0x8000) selected_part_type = 4; // Steering

        // Place part
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            if (selected_part_type >= 0 && selected_part_type < part_templates.size()) {
                auto new_part = std::make_shared<VehiclePart>(*part_templates[selected_part_type]);
                // Create a new unique ID for this part
                static int next_id = 100;
                new_part->id = next_id++;
                vehicle.addPart(new_part, cursor_x, cursor_y);
            }
        }

        // Remove part
        if (GetAsyncKeyState(VK_BACK) & 0x8000) {
            // Find part at cursor position
            for (const auto& part : vehicle.getParts()) {
                if (part->x_offset == cursor_x && part->y_offset == cursor_y) {
                    vehicle.removePart(part->id);
                    break;
                }
            }
        }

        // Exit build mode
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            vehicle.exitBuildMode();
        }
    }

    void render(CHAR_INFO*& screen, int player_x, int player_y, int screen_width, int screen_height) {
        int half_screen_width = screen_width / 2;
        int half_screen_height = screen_height / 2;

        // Display building grid
        for (int y = -10; y <= 10; y++) {
            for (int x = -10; x <= 10; x++) {
                int screen_x = vehicle.x + x - player_x + half_screen_width;
                int screen_y = vehicle.y + y - player_y + half_screen_height;

                if (screen_x >= 0 && screen_x < screen_width && screen_y >= 0 && screen_y < screen_height) {
                    // Display grid dots
                    if (x == 0 && y == 0) {
                        // Origin point
                        screen[screen_y * screen_width + screen_x].Char.UnicodeChar = '+';
                        screen[screen_y * screen_width + screen_x].Attributes = FG_WHITE;
                    }
                    else {
                        screen[screen_y * screen_width + screen_x].Char.UnicodeChar = '.';
                        screen[screen_y * screen_width + screen_x].Attributes = FG_DARK_GREY;
                    }
                }
            }
        }

        // Draw vehicle parts
        vehicle.render(screen, player_x, player_y, screen_width, screen_height);

        // Display cursor
        int cursor_screen_x = vehicle.x + cursor_x - player_x + half_screen_width;
        int cursor_screen_y = vehicle.y + cursor_y - player_y + half_screen_height;

        if (cursor_screen_x >= 0 && cursor_screen_x < screen_width &&
            cursor_screen_y >= 0 && cursor_screen_y < screen_height) {
            screen[cursor_screen_y * screen_width + cursor_screen_x].Attributes =
                FG_WHITE | BG_DARK_BLUE; // Highlight cursor
        }

        // Display UI
        std::wstring part_name = L"None";
        if (selected_part_type >= 0 && selected_part_type < part_templates.size()) {
            part_name = part_templates[selected_part_type]->name;
        }

        std::wstring ui_text = L"Build Mode - Selected: " + part_name +
            L" | ARROWS: Move | 1-5: Part Type | SPACE: Place | BACKSPACE: Remove | ESC: Exit";

        int ui_y = screen_height - 1;
        for (int i = 0; i < ui_text.length() && i < screen_width; i++) {
            screen[ui_y * screen_width + i].Char.UnicodeChar = ui_text[i];
            screen[ui_y * screen_width + i].Attributes = FG_WHITE | BG_BLUE;
        }
    }
};