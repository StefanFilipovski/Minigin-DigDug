#pragma once
#include "EnemyComponent.h"
#include <memory>
#include <string>
#include <SDL3/SDL_pixels.h>

namespace dae
{
	class AnimationSet;

	// Color key shared by all Dig Dug sprite sheets (the red background)
	inline constexpr SDL_Color SpriteSheetColorKey{ 108, 7, 0, 255 };

	// Type Object (Game Programming Patterns): everything that differs
	// between enemy kinds lives in one table entry instead of code branches.
	// Adding a new enemy kind means adding data here, not new if/else blocks.
	struct EnemyTypeInfo
	{
		EnemyType type{ EnemyType::Pooka };
		std::string walkTexture;
		std::string fireTexture;            // empty = cannot breathe fire
		float moveSpeed{ 2.f };
		bool horizontalKillBonus{ false };  // 2x score when killed from the side
		std::shared_ptr<const AnimationSet> animations; // shared flyweight
	};

	// Returns the cached info for a type, building (and pre-caching textures
	// for) the shared animation sets on first use. Per-animation render sizes
	// depend on cellSize, so the cache rebuilds if a new cell size is passed.
	const EnemyTypeInfo& GetEnemyTypeInfo(EnemyType type, float cellSize);
}
