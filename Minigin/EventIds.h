#pragma once
#include <cstdint>

namespace dae
{
	// SDBM hash
	using EventId = unsigned int;

	template <int length> struct sdbm_hash
	{
		consteval static unsigned int _calculate(const char* const text, unsigned int& value)
		{
			const unsigned int character = sdbm_hash<length - 1>::_calculate(text, value);
			value = character + (value << 6) + (value << 16) - value;
			return text[length - 1];
		}
		consteval static unsigned int calculate(const char* const text)
		{
			unsigned int value = 0;
			const auto character = _calculate(text, value);
			return character + (value << 6) + (value << 16) - value;
		}
	};

	template <> struct sdbm_hash<1>
	{
		consteval static unsigned int _calculate(const char* const text, unsigned int&)
		{
			return text[0];
		}
	};

	template <size_t N> consteval EventId make_sdbm_hash(const char(&text)[N])
	{
		return sdbm_hash<N - 1>::calculate(text);
	}

	// Engine events
	inline constexpr EventId EVENT_PLAYER_DIED = make_sdbm_hash("PlayerDied");
	inline constexpr EventId EVENT_POINTS_GAINED = make_sdbm_hash("PointsGained");

	// Dig Dug game events
	inline constexpr EventId EVENT_ENEMY_KILLED = make_sdbm_hash("EnemyKilled");
	inline constexpr EventId EVENT_ALL_ENEMIES_DEAD = make_sdbm_hash("AllEnemiesDead");
	inline constexpr EventId EVENT_GAME_OVER = make_sdbm_hash("GameOver");
	inline constexpr EventId EVENT_LEVEL_COMPLETE = make_sdbm_hash("LevelComplete");
	inline constexpr EventId EVENT_ROCK_DROPPED = make_sdbm_hash("RockDropped");
}