#pragma once
#include "EnemyComponent.h"
#include <memory>
#include <string>
#include <SDL3/SDL_pixels.h>

namespace dae
{
	class AnimationSet;

	// Color key shared by all Dig Dug sprite sheets
	inline constexpr SDL_Color SpriteSheetColorKey{ 108, 7, 0, 255 };

	// Type Object
	struct EnemyTypeInfo
	{
		EnemyType type{ EnemyType::Pooka };
		std::string walkTexture;
		std::string fireTexture;            
		float moveSpeed{ 2.f };
		bool horizontalKillBonus{ false };  
		std::shared_ptr<const AnimationSet> animations;
	};

	// Returns the cached info for a type, building (and pre-caching textures
	// for) the shared animation sets on first use. Per-animation render sizes
	// depend on cellSize, so the cache rebuilds if a new cell size is passed.
	const EnemyTypeInfo& GetEnemyTypeInfo(EnemyType type, float cellSize);
}
