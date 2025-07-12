#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <string>
#include <array>
#include "animation.h"
#include "game_object.h"
#include <format>
using namespace std;

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;
const int MAP_ROWS = 10;
const int MAP_COLS = 20;
const int TILE_SIZE = 32;

struct SDLState
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	int width, height, logW, logH;
	const bool *keys;
	bool mouseClick;
	bool fullScreen;
	SDLState() : keys(SDL_GetKeyboardState(nullptr)) 
	{
		fullScreen = false;
	}

};

struct Resources
{
	const int ANIM_PLAYER_IDLE = 0;
	const int ANIM_PLAYER_RUN = 1;
	vector<Animation> playerAnims;
	const int ANIM_SPEAR_MOVING = 0;
	const int ANIM_SPEAR_HIT = 1;
	vector<Animation> spearAnims;

	vector<SDL_Texture *> textures;
	SDL_Texture *texDiverStanding;
	SDL_Texture *texDiverRunning;
	SDL_Texture *texBoat;
	SDL_Texture *texShallowWater;
	SDL_Texture *texMediumWater;
	SDL_Texture *texDeepWater;
	SDL_Texture *texRock;
	SDL_Texture *texSurface;
	SDL_Texture *texTreasure;
	SDL_Texture *texSpear;
	SDL_Texture *texSpearHit;

	SDL_Texture *loadTexture(SDL_Renderer *renderer, const string& filepath)
	{
		SDL_Texture *tex = IMG_LoadTexture(renderer, filepath.c_str());
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
		textures.push_back(tex);
		return tex;
	}

	void load(SDLState& state)
	{
		playerAnims.resize(5);
		playerAnims[ANIM_PLAYER_IDLE] = Animation(1, 0.5f);
		playerAnims[ANIM_PLAYER_RUN] = Animation(2, 0.5f);
		spearAnims.resize(2);
		spearAnims[ANIM_SPEAR_MOVING] = Animation(1, 0.05f);
		spearAnims[ANIM_SPEAR_HIT] = Animation(3, 0.15f);

		texDiverStanding = loadTexture(state.renderer, "res/diver_standing.png");
		texDiverRunning = loadTexture(state.renderer, "res/diver_running.png");
		texBoat = loadTexture(state.renderer, "res/boat.png");
		texShallowWater = loadTexture(state.renderer, "res/shallow_water.png");
		texMediumWater = loadTexture(state.renderer, "res/medium_water.png");
		texDeepWater = loadTexture(state.renderer, "res/deep_water.png");
		texRock = loadTexture(state.renderer, "res/rock.png");
		texSurface = loadTexture(state.renderer, "res/water_surface.png");
		texTreasure = loadTexture(state.renderer, "res/treasure.png");
		texSpear = loadTexture(state.renderer, "res/spear.png");
		texSpearHit = loadTexture(state.renderer, "res/spear_hit.png");
	}

	void unload()
	{
		for (SDL_Texture *tex : textures)
		{
			SDL_DestroyTexture(tex);
		}
	}
};

struct GameState
{
	array<vector<GameObject>, 2> layers;
	vector<GameObject> backgroundTiles;
	vector<GameObject> spears;
	//vector<GameObject> foregroundTiles;
	int playerIndex;
	SDL_FRect mapViewport;
	bool debugMode;

	GameState(const SDLState &state)
	{
		playerIndex = -1;
		mapViewport = SDL_FRect{
			.x = 0,
			.y = 0,
			.w = static_cast<float>(state.logW),
			.h = static_cast<float>(state.logH)
		};
		debugMode = false;
	}
	
	GameObject &player() { return layers[LAYER_IDX_CHARACTERS][playerIndex]; }
};

void cleanup(SDLState &state);
bool initialize(SDLState& state);
void drawObject(const SDLState &state, GameState &gs, GameObject &obj, float width, float height, float deltaTime);
void update(const SDLState &state, GameState &gs, Resources &res, GameObject &obj, float deltaTime);
void createTiles(const SDLState &state, GameState &gs, const Resources &res);
void checkCollision(const SDLState &state, GameState &gs, Resources &res, GameObject &a, GameObject &b, float deltaTime);
void collisionResponse(const SDLState &state, GameState &gs, Resources &res,
	const SDL_FRect &rectA, const SDL_FRect &rectB, const SDL_FRect &rectC,
	GameObject &objA, GameObject &objB, float deltaTime);
void handleKeyInput(const SDLState &state, GameState &gs, GameObject &obj,
	SDL_Scancode key, bool keyDown);
void handleMouseInput(const SDLState &state, GameState &gs, GameObject &obj,
	SDL_MouseButtonEvent mouse, bool mouseDown);