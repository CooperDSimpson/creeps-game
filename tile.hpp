#pragma once
#include <iostream>;
#include <cmath>;
#include <string>;
#include <Windows.h>
#include "colors.hpp";

struct Tile {
public:
    wchar_t map_revealed_graphic = '.';


    int map_revealed_graphic_color = FG_WHITE;

    bool empty_ = false;
    bool solid = false;
    bool player_passthrough = true;
    bool enemy_passthrough = true;
};
