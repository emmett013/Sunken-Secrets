#pragma once
#include "glm/glm.hpp"
#include "vector"
#include <SDL3/SDL.h>
#include "animation.h"

enum class PlayerState
{
	idle, running, jumping
};

enum class SpearState
{
	moving, colliding, inactive
};

struct PlayerData
{
	PlayerState state;
	Timer weaponTimer;	

	PlayerData() : weaponTimer(0.2f)	// fire rate 0.2 seconds
	{
		state = PlayerState::idle;
	}
};

struct LevelData
{

};

struct EnemyData
{

};

struct SpearData
{
	SpearState state;
	SpearData() : state(SpearState::moving)
	{
	}
};
union ObjectData
{
	PlayerData player;
	LevelData level;
	EnemyData enemy;
	SpearData spear;
};

enum class ObjectType
{
	player, level, enemy, spear
};

struct GameObject
{
	ObjectType type;
	ObjectData data;
	glm::vec2 position, velocity, acceleration;
	float direction;
	float maxSpeedX;
	std::vector<Animation> animations;
	int currentAnimation;
	SDL_Texture *texture;
	bool dynamic;
	bool grounded;
	SDL_FRect collider;

	GameObject() : data{.level = LevelData()}, collider{ 0 }
	{
		type = ObjectType::level;
		direction = 1;
		maxSpeedX = 0;
		position = velocity = acceleration = glm::vec2(0);
		currentAnimation = -1;
		texture = nullptr;
		dynamic = false;
		grounded = false;
	}

};