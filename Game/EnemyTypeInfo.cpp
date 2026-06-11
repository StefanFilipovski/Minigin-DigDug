#include "EnemyTypeInfo.h"
#include "SpriteAnimatorComponent.h"
#include "ResourceManager.h"
#include <iterator>

namespace dae
{
	namespace
	{
		// Walk sheets: row 0 = walk right, row 1 = walk left, 3 frames each
		constexpr int FrameW = 13;
		constexpr int FrameH = 14;
		constexpr int Stride = 15; // 13px frame + 2px gap

		void PrecacheTextures(const char* const* names, size_t count)
		{
			for (size_t i = 0; i < count; ++i)
				ResourceManager::GetInstance().LoadTexture(names[i], SpriteSheetColorKey);
		}

		std::shared_ptr<const AnimationSet> BuildPookaAnimations(float cellSize)
		{
			static const char* const textures[]{
				"Enemy1Walk.png", "Enemy1Ghost.png",
				"Enemy1Explode1.png", "Enemy1Explode2.png",
				"Enemy1Explode3.png", "Enemy1Explode4.png" };
			PrecacheTextures(textures, std::size(textures));

			auto set = std::make_shared<AnimationSet>();

			set->Add("walk_right", "Enemy1Walk.png",
				{ {0, 0, FrameW, FrameH}, {Stride, 0, FrameW, FrameH}, {Stride * 2, 0, FrameW, FrameH} }, 6.f);
			set->Add("walk_left", "Enemy1Walk.png",
				{ {0, Stride, FrameW, FrameH}, {Stride, Stride, FrameW, FrameH}, {Stride * 2, Stride, FrameW, FrameH} }, 6.f);

			// No up/down anims; Play("walk_up/down") no-ops and keeps the last one

			constexpr int GhostW = 14;
			constexpr int GhostH = 8;
			set->Add("ghost", "Enemy1Ghost.png",
				{ {0, 0, GhostW, GhostH}, {GhostW, 0, GhostW, GhostH} }, 6.f);
			set->SetRenderSize("ghost", cellSize * 0.6f, cellSize * 0.6f);

			// Inflate stages: one sprite per file, faces left (flipped when hit from the right)
			set->Add("inflate_1", "Enemy1Explode1.png", { {0, 0, 14, 14} }, 1.f, false);
			set->Add("inflate_2", "Enemy1Explode2.png", { {0, 0, 20, 16} }, 1.f, false);
			set->Add("inflate_3", "Enemy1Explode3.png", { {0, 0, 21, 20} }, 1.f, false);
			set->Add("inflate_4", "Enemy1Explode4.png", { {0, 0, 25, 20} }, 1.f, false);

			return set;
		}

		std::shared_ptr<const AnimationSet> BuildFygarAnimations(float cellSize)
		{
			static const char* const textures[]{
				"Enemy2Walk.png", "Enemy2Ghost.png", "Enemy2Fire.png",
				"Enemy2Explode1.png", "Enemy2Explode2.png",
				"Enemy2Explode3.png", "Enemy2Explode4.png" };
			PrecacheTextures(textures, std::size(textures));

			auto set = std::make_shared<AnimationSet>();

			// Same sheet layout as the Pooka
			set->Add("walk_right", "Enemy2Walk.png",
				{ {0, 0, FrameW, FrameH}, {Stride, 0, FrameW, FrameH}, {Stride * 2, 0, FrameW, FrameH} }, 6.f);
			set->Add("walk_left", "Enemy2Walk.png",
				{ {0, Stride, FrameW, FrameH}, {Stride, Stride, FrameW, FrameH}, {Stride * 2, Stride, FrameW, FrameH} }, 6.f);

			constexpr int GhostW = 14;
			constexpr int GhostH = 11;
			set->Add("ghost", "Enemy2Ghost.png",
				{ {0, 0, GhostW, GhostH}, {GhostW + 1, 0, GhostW, GhostH} }, 6.f);
			set->SetRenderSize("ghost", cellSize * 0.6f, cellSize * 0.6f);

			// Inflate stages from individual sheets, faces left by default
			set->Add("inflate_1", "Enemy2Explode1.png", { {0, 0, 15, 13} }, 1.f, false);
			set->Add("inflate_2", "Enemy2Explode2.png", { {0, 0, 20, 19} }, 1.f, false);
			set->Add("inflate_3", "Enemy2Explode3.png", { {0, 0, 20, 21} }, 1.f, false);
			set->Add("inflate_4", "Enemy2Explode4.png", { {0, 0, 24, 23} }, 1.f, false);

			return set;
		}
	}

	const EnemyTypeInfo& GetEnemyTypeInfo(EnemyType type, float cellSize)
	{
		static float cachedCellSize = -1.f;
		static EnemyTypeInfo pooka;
		static EnemyTypeInfo fygar;

		if (cachedCellSize != cellSize)
		{
			cachedCellSize = cellSize;
			pooka = { EnemyType::Pooka, "Enemy1Walk.png", "", 2.5f, false,
				BuildPookaAnimations(cellSize) };
			fygar = { EnemyType::Fygar, "Enemy2Walk.png", "Enemy2Fire.png", 2.0f, true,
				BuildFygarAnimations(cellSize) };
		}

		return (type == EnemyType::Fygar) ? fygar : pooka;
	}
}
