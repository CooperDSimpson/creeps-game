#pragma once

#include <vector>
#include <Windows.h>
#include "colors.hpp"
#include "tile.hpp"
#include "tiletypes.hpp"

class World {
public:
    std::vector<std::vector<Tile>> grid;
    int world_width = 200;
    int world_height = 200;

    World() = default;

    // Pre-allocate memory for the grid to avoid reallocations
    void build() {
        grid.clear();
        grid.reserve(world_height);
        
        for (int y = 0; y < world_height; y++) {
            grid.emplace_back(world_width, empty);
        }
    }
};