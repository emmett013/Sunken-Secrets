// sunken-secrets.cpp : Defines the entry point for the application.
//
#include "sunken-secrets.h"

int main(int argc, char *argv[])
{
	SDLState state;
	state.width = 1600;
	state.height = 900;
	state.logW = 640;
	state.logH = 320;
	state.mouseClick = false;

	if (!initialize(state))
	{
		return 1;
	}

	// load game assets
	Resources res;
	res.load(state);

	// setup game data
	//
	GameState gs(state);
	createTiles(state, gs, res);
	uint64_t prevTime = SDL_GetTicks();

	// start game loop
	bool running{ true };
	while (running)
	{
		uint64_t currTime = SDL_GetTicks();
		float deltaTime = (currTime - prevTime) / 1000.0f; // loop time in seconds
		if (deltaTime == 0.0f)
		{
			continue;
		}
		prevTime = currTime;

		SDL_Event event{ 0 };
		bool jumped = false;

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_EVENT_QUIT:
				{
					running = false;
					break;
				}
				case SDL_EVENT_WINDOW_RESIZED:
				{
					state.width = event.window.data1;
					state.height = event.window.data2;
					break;
				}
				case SDL_EVENT_KEY_DOWN:
				{
					handleKeyInput(state, gs, gs.player(), event.key.scancode, true);
					break;
				}
				case SDL_EVENT_KEY_UP:
				{
					handleKeyInput(state, gs, gs.player(), event.key.scancode, false);
					if (event.key.scancode == SDL_SCANCODE_F1)
					{
						gs.debugMode = !gs.debugMode;
					}
					if (event.key.scancode == SDL_SCANCODE_F11)
					{
						state.fullScreen = !state.fullScreen;
						SDL_SetWindowFullscreen(state.window, state.fullScreen);
					}
					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				{
					state.mouseClick = true;
					//handleMouseInput(state, gs, gs.player(), event.button, true);
					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_UP:
				{
					state.mouseClick = false;
					//handleMouseInput(state, gs, gs.player(), event.button, false);
					break;
				}

			}
		}

		// update all objects
		for (auto &layer : gs.layers)
		{
			for (GameObject &obj : layer)
			{
				update(state, gs, res, obj, deltaTime);

				// update the animation
				if (obj.currentAnimation != -1)
				{
					obj.animations[obj.currentAnimation].step(deltaTime);
				}
			}
		}

		// update spears
		for (GameObject &spear : gs.spears)
		{
			update(state, gs, res, spear, deltaTime);

			// update the animation
			if (spear.currentAnimation != -1)
			{
				spear.animations[spear.currentAnimation].step(deltaTime);
			}
		}
		// calculate viewport position
		gs.mapViewport.x = (gs.player().position.x + TILE_SIZE / 2) - gs.mapViewport.w / 2;

		// perform drawing commands
		SDL_SetRenderDrawColor(state.renderer, 188, 245, 255, 255);
		SDL_RenderClear(state.renderer);

		// draw all  background objects
		for (GameObject &obj : gs.backgroundTiles)
		{
			SDL_FRect dst{
				.x = obj.position.x - gs.mapViewport.x,
				.y = obj.position.y,
				.w = static_cast<float>(TILE_SIZE),
				.h = static_cast<float>(TILE_SIZE)
			};
			SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
		}
		// draw all objects
		for (auto &layer : gs.layers)
		{
			for (GameObject &obj : layer)
			{
				drawObject(state, gs, obj, TILE_SIZE, TILE_SIZE, deltaTime);
			}
		}

		// draw spears
		for (GameObject &spear : gs.spears)
		{
			if (spear.data.spear.state != SpearState::inactive)
			{
				drawObject(state, gs, spear, TILE_SIZE, TILE_SIZE, deltaTime);
			}
		}

		SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255);
		// display debug
		if (gs.debugMode)
		{
			SDL_RenderDebugText(state.renderer, 5, 5,
				format("State: {} Velocity Y: {} dTime: {}",
						static_cast<int>(gs.player().data.player.state), gs.player().velocity.y, deltaTime).c_str());
		}
		// swap buffers and present
		SDL_RenderPresent(state.renderer);
	}

	res.unload();
	cleanup(state);
	return 0;
}

bool initialize(SDLState &state)
{
	bool initSuccess = true;

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", nullptr);
		initSuccess = false;
	}

	// create window
	state.window = SDL_CreateWindow("Treasure Dive", state.width, state.height, SDL_WINDOW_RESIZABLE);
	if (!state.window)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", nullptr);
		cleanup(state);
		initSuccess = false;
	}

	// create renderer
	state.renderer = SDL_CreateRenderer(state.window, nullptr);
	if (!state.renderer)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating renderer", state.window);
		initSuccess = false;
	}

	// enable vsync
	SDL_SetRenderVSync(state.renderer, 1);
	// configure resolution
	SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	return initSuccess;
}

void cleanup(SDLState &state)
{
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
}

void drawObject(const SDLState &state, GameState &gs, GameObject &obj, float width, float height, float deltaTime)
{
	float srcX = obj.currentAnimation != -1 
		? obj.animations[obj.currentAnimation].currentFrame() * width : 0.0f;
	SDL_FRect src{
		.x = srcX,
		.y = 0,
		.w = width,
		.h = height
	};

	SDL_FRect dst{
		.x = obj.position.x - gs.mapViewport.x,
		.y = obj.position.y,
		.w = width,
		.h = height
	};

	SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);

	if (gs.debugMode)
	{
		SDL_FRect rectA
		{
			.x = obj.position.x + obj.collider.x - gs.mapViewport.x,
			.y = obj.position.y + obj.collider.y,
			.w = obj.collider.w,
			.h = obj.collider.h
		};
		SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 100);
		SDL_RenderFillRect(state.renderer, &rectA);
		SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_NONE);
	}
}

void update(const SDLState &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime)
{
	// apply gravity
	if (obj.dynamic && !obj.grounded)
	{
		obj.velocity += glm::vec2(0, 500) * deltaTime;
	}

	float currentDirection = 0.0f;
	if (obj.type == ObjectType::player)
	{
		if (state.keys[SDL_SCANCODE_A])
		{
			currentDirection += -1.0f;
		}
		if (state.keys[SDL_SCANCODE_D])
		{
			currentDirection += 1.0f;
		}

		Timer &weaponTimer = obj.data.player.weaponTimer;
		weaponTimer.step(deltaTime);

		const auto handleShooting = [&state, &gs, &res, &obj, &weaponTimer]()
		{
			if (state.mouseClick) //(state.mouse == SDL_BUTTON_LEFT)	// shoot spear
			{
				if (weaponTimer.isTimeout())
				{
					weaponTimer.reset();
					GameObject spear;
					spear.data.spear = SpearData();
					spear.type = ObjectType::spear;
					spear.direction = gs.player().direction;
					spear.texture = res.texSpear;
					spear.collider = SDL_FRect{
						.x = (obj.direction < 0 ? 20.0f : 0.0f),
						.y = 13.0f,
						.w = 12.0f,
						.h = 5.0f,
					};
					const int yVariation = 15;
					const float yVelocity = SDL_rand(yVariation) - yVariation / 2.0f;
					spear.velocity = glm::vec2(200.0f * obj.direction, yVelocity);
					spear.maxSpeedX = 1000.0f;
					spear.animations = res.spearAnims;

					// adjust spear start position
					const float left = -10.0f;
					const float right = 10.0f;
					spear.position = glm::vec2(
						obj.position.x + (obj.direction < 0 ? left : right),
						obj.position.y
					);
					// overwrite inactive spear if possible to save space
					bool foundInactive = false;
					for (int i = 0; i < gs.spears.size() && !foundInactive; i++)
					{
						if (gs.spears[i].data.spear.state == SpearState::inactive)
						{
							foundInactive = true;
							gs.spears[i] = spear;
						}
					}
					if (!foundInactive)
					{
						gs.spears.push_back(spear);
					}
				}
			}
		};

		switch (obj.data.player.state)
		{
			case PlayerState::idle:		// switch to idle state
			{
				if (currentDirection)
				{
					obj.data.player.state = PlayerState::running;
				}
				else
				{
					// decelarate
					if (obj.velocity.x)
					{
						const float factor = obj.velocity.x > 0 ? -1.5f : 1.5f;
						float amount = factor * obj.acceleration.x * deltaTime;
						if (abs(obj.velocity.x) < abs(amount))
						{
							obj.velocity.x = 0;
						}
						else
						{
							obj.velocity.x += amount;
						}
					}
				}
				handleShooting();
				obj.texture = res.texDiverStanding;
				obj.currentAnimation = res.ANIM_PLAYER_IDLE;
				break;
			}
			case PlayerState::running:	// switch to running state
			{
				if (!currentDirection)
				{
					obj.data.player.state = PlayerState::idle;
				}
				handleShooting();
				obj.texture = res.texDiverRunning;
				obj.currentAnimation = res.ANIM_PLAYER_RUN;
				break;

			}
			case PlayerState::jumping:	// switch to jumping state
			{
				handleShooting();
				obj.texture = res.texDiverStanding;
				obj.currentAnimation = res.ANIM_PLAYER_IDLE;
				break;
			}
		}
	}
	else if (obj.type == ObjectType::spear)
	{
		switch (obj.data.spear.state)
		{
			case SpearState::moving:
			{
				if (obj.position.x - gs.mapViewport.x < 0 ||	// if spear is off screen
					obj.position.x - gs.mapViewport.x > state.logW ||
					obj.position.y - gs.mapViewport.y < 0 ||
					obj.position.y - gs.mapViewport.y > state.logH)
				{
					obj.data.spear.state = SpearState::inactive;
				}
				break;
			}
			case SpearState::colliding:
			{
				if (obj.animations[obj.currentAnimation].isDone())
				{
					obj.data.spear.state = SpearState::inactive;
				}
				break;
			}

		}
	}
	if (currentDirection)
	{
		obj.direction = currentDirection;
	}

	// add acceleration to velocity
	obj.velocity += currentDirection * obj.acceleration * deltaTime;
	if (abs(obj.velocity.x) > obj.maxSpeedX)
	{
		obj.velocity.x = currentDirection * obj.maxSpeedX;
	}

	// add velocity to position
	obj.position += obj.velocity * deltaTime;

	// handle collision detection
	bool foundGround = false;
	for (auto &layer : gs.layers)
	{
		for (GameObject &objB : layer)
		{
			if (&obj != &objB)
			{
				checkCollision(state, gs, res, obj, objB, deltaTime);

				// grounded sensor
				if (objB.type != ObjectType::level)	// only check grounded against level
				{
					continue;
				}
				SDL_FRect sensor{
					.x = obj.position.x + obj.collider.x,
					.y = obj.position.y + obj.collider.y + obj.collider.h,
					.w = obj.collider.w,
					.h = 1
				};
				SDL_FRect rectB{
					.x = objB.position.x + objB.collider.x,
					.y = objB.position.y + objB.collider.y,
					.w = objB.collider.w,
					.h = objB.collider.h
				};
				SDL_FRect result{ 0 };

				if (SDL_GetRectIntersectionFloat(&sensor, &rectB, &result))	// use Get because of bug in HasRectIntersectionFloat
				{
					foundGround = true;
				}
			}
		}
	}
 	if (obj.grounded != foundGround)
	{
		obj.grounded = foundGround;
		if (foundGround && (obj.type == ObjectType::player))
		{
			obj.data.player.state = PlayerState::running;
		}
	}
}


void checkCollision(const SDLState &state, GameState &gs, Resources &res, GameObject &a, GameObject &b, float deltaTime)
{
	SDL_FRect rectA
	{
		.x = a.position.x + a.collider.x,
		.y = a.position.y + a.collider.y,
		.w = a.collider.w,
		.h = a.collider.h
	};
	SDL_FRect rectB
	{
		.x = b.position.x + b.collider.x,
		.y = b.position.y + b.collider.y,
		.w = b.collider.w,
		.h = b.collider.h
	};
	SDL_FRect rectC{ 0 };

	if (SDL_GetRectIntersectionFloat(&rectA, &rectB, &rectC))
	{
		// intersection found
		collisionResponse(state, gs, res, rectA, rectB, rectC, a, b, deltaTime);
	}
}

void collisionResponse(const SDLState &state, GameState &gs, Resources &res, 
	const SDL_FRect &rectA, const SDL_FRect &rectB, const SDL_FRect &rectC,
	GameObject &objA, GameObject &objB, float deltaTime)
{
	const auto genericResponse = [&]()
	{
		{
			if (rectC.w < rectC.h)
			{
				// horizontal collision
				if (objA.velocity.x > 0) // going right
				{
					objA.position.x -= rectC.w;
				}
				else if (objA.velocity.x < 0) // going left
				{
					objA.position.x += rectC.w;
				}
				objA.velocity.x = 0;
			}
			else
			{
				// vertical collision
				if (objA.velocity.y > 0) // going down
				{
					objA.position.y -= rectC.h;
				}
				else if (objA.velocity.y < 0) // going up
				{
					objA.position.y += rectC.h;
				}
				objA.velocity.y = 0;
			}
		}
	};
	if (objA.type == ObjectType::player)
	{
		switch (objB.type)
		{
			case ObjectType::level:
			{
				genericResponse();
				break;
			}
		}
	}
	else if (objA.type == ObjectType::spear && objB.type != ObjectType::player)
	{
		switch (objA.data.spear.state)
		{
			case SpearState::moving:
			{
				genericResponse();
				objA.velocity *= 0;
				objA.data.spear.state = SpearState::colliding;
				objA.texture = res.texSpearHit;
				objA.currentAnimation = res.ANIM_SPEAR_HIT;
				break;
			}
		}
	}
}
void createTiles(const SDLState &state, GameState &gs, const Resources &res)
{
	/*
	*	1 - Player
	*	2 - Boat
	*	3 - Surface
	*	4 - Shallow Water
	*	5 - Medium Water
	*   6 - Deep Water
	*	7 - Rock
	*	8 - Treasure
	*/
	short map[MAP_ROWS][MAP_COLS] = {
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 7, 0, 7, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0 },
		{ 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 }
	};
	short background[MAP_ROWS][MAP_COLS] = {
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 },
		{ 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
		{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
		{ 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 },
		{ 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
	};

	const auto loadMap = [&state, &gs, &res](short layer[MAP_ROWS][MAP_COLS])
		{
			const auto createObject = [&state](int r, int c, SDL_Texture *tex, ObjectType type)
				{
					GameObject o;
					o.type = type;
					o.position = glm::vec2(c * TILE_SIZE, state.logH - (MAP_ROWS - r) * TILE_SIZE);
					o.texture = tex;
					o.collider = { .x = 0, .y = 0, .w = TILE_SIZE, .h = TILE_SIZE };
					return o;
				};
			for (int r = 0; r < MAP_ROWS; r++)
			{
				for (int c = 0; c < MAP_COLS; c++)
				{
					switch (layer[r][c])
					{
					case 1:	// player
					{
						// create player
						GameObject player = createObject(r, c, res.texDiverStanding, ObjectType::player);
						player.data.player = PlayerData();
						player.animations = res.playerAnims;
						player.currentAnimation = res.ANIM_PLAYER_IDLE;
						player.acceleration = glm::vec2(300, 0);
						player.maxSpeedX = 100;
						player.dynamic = true;
						player.collider = { .x = 11, .y = 6, .w = 10, .h = 20 };
						gs.layers[LAYER_IDX_CHARACTERS].push_back(player);
						gs.playerIndex = gs.layers[LAYER_IDX_CHARACTERS].size() - 1;
						break;
					}
					case 2:	// boat
					{
						GameObject boat = createObject(r, c, res.texBoat, ObjectType::level);
						boat.collider.y = 30.0f;
						boat.collider.h = 2.0f;
						gs.layers[LAYER_IDX_LEVEL].push_back(boat);
						break;
					}
					case 3:	// surface
					{
						GameObject surface = createObject(r, c, res.texSurface, ObjectType::level);
						gs.backgroundTiles.push_back(surface);
						break;
					}
					case 4:	// shallow water
					{
						GameObject shallowWater = createObject(r, c, res.texShallowWater, ObjectType::level);
						gs.backgroundTiles.push_back(shallowWater);
						break;
					}
					case 5:	// medium water
					{
						GameObject mediumWater = createObject(r, c, res.texMediumWater, ObjectType::level);
						gs.backgroundTiles.push_back(mediumWater);
						break;
					}
					case 6:	// deep water
					{
						GameObject deepWater = createObject(r, c, res.texDeepWater, ObjectType::level);
						gs.backgroundTiles.push_back(deepWater);
						break;
					}
					case 7:	// rock
					{
						GameObject rock = createObject(r, c, res.texRock, ObjectType::level);
						gs.layers[LAYER_IDX_LEVEL].push_back(rock);
						break;
					}
					case 8:	//treasure
					{
						GameObject treasure = createObject(r, c, res.texTreasure, ObjectType::level);
						gs.layers[LAYER_IDX_LEVEL].push_back(treasure);
						break;
					}
					}
				}
			}
		};
	loadMap(map);
	loadMap(background);
	assert(gs.playerIndex != -1);

}

void handleKeyInput(const SDLState &state, GameState &gs, GameObject &obj,
	SDL_Scancode key, bool keyDown)
{
	const float JUMP_FORCE = -200.0f;

	if (obj.type == ObjectType::player)
	{
		switch (obj.data.player.state)
		{
			case PlayerState::idle:
			{
				if (key == SDL_SCANCODE_SPACE && keyDown)
				{
					obj.data.player.state = PlayerState::jumping;
					obj.velocity.y += JUMP_FORCE;
				}
				break;
			}
			case PlayerState::running:
			{
				if (key == SDL_SCANCODE_SPACE && keyDown)
				{
					obj.data.player.state = PlayerState::jumping;
					obj.velocity.y += JUMP_FORCE;
				}
				break;
			}
		}
	}
}

void handleMouseInput(const SDLState &state, GameState &gs, GameObject &obj,
	SDL_MouseButtonEvent mouse, bool mouseDown)
{

}