#pragma once
#include "EventIds.h"

namespace dae
{
	// Dig Dug game events
	inline constexpr EventId EVENT_ENEMY_KILLED = make_sdbm_hash("EnemyKilled");
	inline constexpr EventId EVENT_ALL_ENEMIES_DEAD = make_sdbm_hash("AllEnemiesDead");
	inline constexpr EventId EVENT_GAME_OVER = make_sdbm_hash("GameOver");
	inline constexpr EventId EVENT_LEVEL_COMPLETE = make_sdbm_hash("LevelComplete");
	inline constexpr EventId EVENT_ROCK_DROPPED = make_sdbm_hash("RockDropped");
	inline constexpr EventId EVENT_ROCK_CRUSH_COMPLETE = make_sdbm_hash("RockCrushComplete");
}
