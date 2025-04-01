#pragma once
#include "entity.hpp"


#include <iostream>
#include <cstdlib>  
#include <ctime>    
#include <string>
#include <vector>
#include <sstream>
#include <Windows.h>

class Player : public Entity {
public:
	Player() {
		is_ghost = false;
		entity_graphic = '@';
		name = L"player";
		MAX_SPEED = 30;
		FRICTION_DECELERATION = 0;
		is_enemy = false;
	}

	unsigned int xp = 0;
	unsigned int level = 1;
	unsigned int score = 0;

	void open_inventory() {
		0 + 0;
	}
};