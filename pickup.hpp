#pragma once

//#include "entity.hpp"
#include "tiletypes.hpp"

class Pickup{
public:
	int x_pos = 0;
	int y_pos = 0;
	bool delete_me = 0;
	Item item = { L"coin", L"none", L'O', 1};
};