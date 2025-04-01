#pragma once;

#include "tile.hpp"

Tile wall = { Tile{.map_revealed_graphic = 0x2588, .solid = 1} };
Tile empty = { Tile{.map_revealed_graphic = L'.', .empty_ = true, .solid = 0} };
Tile door = { Tile{.map_revealed_graphic = L'D', .solid = NULL, .enemy_passthrough = 0} };