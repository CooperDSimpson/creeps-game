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
#include "colors.hpp"
#include "world.hpp"
#include "player.hpp"
#include "creep.hpp"
#include "tile.hpp"
#include "tiletypes.hpp"
#include "pickup.hpp"
#include "churchill.hpp"

// Aspect ratio 16:9
int screen_width = 157;
int screen_height = 48;

int status_size = 0;

int delta_time = 0;
int delta_x;
int delta_y;

int movespeed = 30;

const int MAX_AGGRESSION = 10;
int aggression = 0;
bool go_thread = true;
bool angered = false;

void doTestScreen(CHAR_INFO*& screen) {
	for (int y = 0; y < screen_height + status_size; y++) {
		for (int x = 0; x < screen_width; x++) {
			screen[y * screen_width + x].Char.UnicodeChar = '#';
			screen[y * screen_width + x].Attributes = BG_RED;
			screen[y * screen_width + x].Attributes = FG_BLUE;
		}
	}
}

void loadMapToScreen(CHAR_INFO*& screen, World& world, Player& player, std::vector<Creep>& creeps, std::vector<Pickup>& pickups) {

	int half_screen_width = screen_width / 2;
	int half_screen_height = screen_height / 2;

	int adjusted_screen_height = screen_height - 1;

	for (int y = 0; y < screen_height; y++) {
		for (int x = 0; x < screen_width; x++) {
			// Calculate world coordinates relative to player's position
			int world_x = player.x + (x - half_screen_width);
			int world_y = player.y + (y - half_screen_height);

			if (world_y >= 0 && world_y < world.world_height && world_x >= 0 && world_x < world.world_width) {
				//for (Pickup pickup : pickups) {
				//	if (world_y == pickup.y_pos && world_x == pickup.x_pos) {
				//		screen[y * screen_width + x].Char.UnicodeChar = pickup.item.graphic;
				//		screen[y * screen_width + x].Attributes = FG_YELLOW;
				//	}
				//}

				if (world_y == player.y && world_x == player.x) {
					// Render the player
					screen[y * screen_width + x].Char.UnicodeChar = player.entity_graphic;
					screen[y * screen_width + x].Attributes = player.entity_graphic_color;
				}

				else {
					// Render the world tile
					screen[y * screen_width + x].Char.UnicodeChar = world.grid[world_y][world_x].map_revealed_graphic;
					screen[y * screen_width + x].Attributes = world.grid[world_y][world_x].map_revealed_graphic_color;
				}
			}
			else {
				// Render an out-of-bounds character
				screen[y * screen_width + x].Char.UnicodeChar = '$';
				screen[y * screen_width + x].Attributes = FG_RED;
			}
		}
	}

	for (Creep creep : creeps) {
		// Convert world position to screen position
		int screen_x = creep.x - player.x + half_screen_width;
		int screen_y = creep.y - player.y + half_screen_height;

		// Ensure the creep is within screen bounds before drawing
		if (screen_x >= 0 && screen_x < screen_width && screen_y >= 0 && screen_y < screen_height) {
			screen[screen_y * screen_width + screen_x].Char.UnicodeChar = creep.entity_graphic;
			screen[screen_y * screen_width + screen_x].Attributes = creep.entity_graphic_color;
		}
	}

	for (Pickup pickup : pickups) {
		int screen_x = pickup.x_pos - player.x + half_screen_width;
		int screen_y = pickup.y_pos - player.y + half_screen_height;

		// Ensure the creep is within screen bounds before drawing
		if (screen_x >= 0 && screen_x < screen_width && screen_y >= 0 && screen_y < screen_height) {
			screen[screen_y * screen_width + screen_x].Char.UnicodeChar = pickup.item.graphic;
			screen[screen_y * screen_width + screen_x].Attributes = FG_YELLOW;
		}
	}

	screen[half_screen_height * screen_width + half_screen_width].Char.UnicodeChar = player.entity_graphic;
	screen[half_screen_height * screen_width + half_screen_width].Attributes = player.entity_graphic_color;

}

int main() {
	int tile_selection = 1;
	int comb_y = 0;
	int comb_x = 0;
	int placement_y = 0;
	int placement_x = 0;
	using clock = std::chrono::high_resolution_clock;
	std::chrono::milliseconds target_frame_time(1000 / 240);  // 240 FPS means 1000 ms / 240 = 4.167 ms
	auto lastTime = clock::now();
	// Setup
	CHAR_INFO* screen = new CHAR_INFO[screen_width * screen_height];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;
	// Setup done
	World world;
	Player player;
	srand(time(0));
	std::vector<Creep> creeps;
	std::vector<Pickup> pickups;

	Creep tempcreep;
	world.build();

	for (int i = 0; i < 1; i++) {
		//tempcreep.set_position(rand() % 1000, rand() % 1000);
		tempcreep.set_position(50, 50);
		tempcreep.wander(world);
		creeps.push_back(tempcreep);
	}


	for (int i = 0; i < 10; i++) {
		pickups.push_back(Pickup{.x_pos = rand() % world.world_width, .y_pos = rand() % world.world_height});
	}

	// Start the mouse tracking thread
	auto currentTime = clock::now();
	Pickup p;
	int i = 0;
	while (true) {
		currentTime = clock::now();
		const std::chrono::duration<double> target_frame_duration(1.0 / 240.0); // Target frame duration for 240 FPS
		std::chrono::duration<double> delta_time = currentTime - lastTime;
		lastTime = currentTime;

		if (0x8000) {
			// Movement logic
			if (true) {

				if (GetAsyncKeyState('1')) {
					tile_selection = 1;
				}
				else if (GetAsyncKeyState('2')) {
					tile_selection = 2;
				}

				//if (GetAsyncKeyState(VK_TAB)) {
				//	player.MAX_SPEED = 60;
				//}
				//else {
				//	player.MAX_SPEED = 120;
				//}

				if (GetAsyncKeyState(VK_TAB)) {
					player.FRICTION_DECELERATION = 500;
					player.MAX_SPEED = 10;
				}
				else {
					player.FRICTION_DECELERATION = 120;
					player.MAX_SPEED = 30;
				}

				if (GetAsyncKeyState('W')) {
					player.y_speed -= 30;
				}
				if (GetAsyncKeyState('S')) {
					player.y_speed += 30;
				}
				if (GetAsyncKeyState('D')) {
					player.x_speed += 30;
				}
				if (GetAsyncKeyState('A')) {
					player.x_speed -= 30;
				}

				if (GetAsyncKeyState(VK_SPACE) || GetAsyncKeyState(VK_SHIFT)) {


					if (GetAsyncKeyState(VK_UP)) { placement_y -= 1; }
					if (GetAsyncKeyState(VK_DOWN)) { placement_y += 1; }
					if (GetAsyncKeyState(VK_LEFT)) { placement_x -= 1; }
					if (GetAsyncKeyState(VK_RIGHT)) { placement_x += 1; }

					comb_x = player.x + placement_x;
					comb_y = player.y + placement_y;

					if (comb_y < world.world_height && comb_y > -1 && comb_x < world.world_width && comb_x > -1 && !(comb_x == player.x && comb_y == player.y)) {
						if (GetAsyncKeyState(VK_SPACE)) {
							if (world.grid[comb_y][comb_x].empty_) {
								switch (tile_selection) {
								case 1: {
									world.grid[comb_y][comb_x] = wall;
									break;
								}
								case 2: {
									world.grid[comb_y][comb_x] = door;
									break;
								}
								}
								aggression++;
							}
						}
						else if (GetAsyncKeyState(VK_SHIFT)) {
							if (!world.grid[comb_y][comb_x].empty_) {
								world.grid[comb_y][comb_x] = empty;
								aggression--;
							}
						}
					}

					comb_x = 0;
					comb_y = 0;
					placement_x = 0;
					placement_y = 0;
				}
				if (GetAsyncKeyState('P')) {
					angered = 0;
					aggression = 0;
				}
				if (GetAsyncKeyState(VK_ESCAPE)) {
					goto end;
				}

			}
		}

		if (aggression > MAX_AGGRESSION) {
			angered = true;
		}
		else {
			angered = false;
		}

		//creeps roll out
		for (int i = 0; i < creeps.size(); i++) {
			creeps.at(i).move(world.world_height, world.world_width, delta_time.count(), world);

			if (angered) {
				creeps.at(i).do_wander = 0;
			}
			else {
				creeps.at(i).do_wander = 1;
			}

			if (player.x == creeps[i].x && player.y == creeps[i].y) {
				goto death;
			}

			creeps.at(i).pursue_position(player.x, player.y, world);
		}
		
		for (int i = 0; i < pickups.size(); i++) {
			p = pickups.at(i);

			if (p.x_pos == player.x && p.y_pos == player.y) {
				pickups.erase(pickups.begin() + i);
				i--;
			}
		}

		player.move(world.world_height, world.world_width, delta_time.count(), world);
		loadMapToScreen(screen, world, player, creeps, pickups);
		COORD bufferSize = { screen_width, screen_height };
		COORD bufferCoord = { 0, 0 };
		SMALL_RECT writeRegion = { 0, 0, screen_width - 1, screen_height - 1 };
		WriteConsoleOutput(hConsole, screen, bufferSize, bufferCoord, &writeRegion);
		i++;
		auto frame_time = std::chrono::high_resolution_clock::now() - currentTime;

		//if (frame_time < target_frame_time) {
		//	// Sleep for the remaining time to ensure we hit the target frame time
		//	std::this_thread::sleep_for(target_frame_time - frame_time);
		//}
		
	}

	death:

	std::wcout << "game over.";

	end:

	return 0;
}