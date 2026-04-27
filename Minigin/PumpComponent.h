#pragma once
#include "Component.h"
#include <memory>
#include <SDL3/SDL.h>
#include "../out/build/x64-debug/_deps/glm-src/glm/ext/vector_int2.hpp"

namespace dae
{
	class GridComponent;
	class GridMovementComponent;
	class SpriteAnimatorComponent;
	class RenderComponent;
	class Texture2D;
	class GameObject;

	class PumpComponent final : public Component
	{
	public:
		PumpComponent(GameObject* pOwner, GridComponent* pGrid);

		void Update(float deltaTime) override;
		void Render() const override;

		// Called by the pump command (button press)
		void Fire();

		bool IsFiring() const { return m_state != PumpState::Idle; }
		bool IsLatched() const { return m_state == PumpState::Latched; }

		void SetRange(int cells) { m_maxRange = cells; }

	private:
		enum class PumpState
		{
			Idle,
			Extending,
			Latched,
			Retracting
		};

		GridComponent* m_pGrid;
		GridMovementComponent* m_pMovement{ nullptr };
		SpriteAnimatorComponent* m_pAnimator{ nullptr };
		bool m_cached{ false };

		PumpState m_state{ PumpState::Idle };
		glm::ivec2 m_fireDirection{ 0, 0 };

		float m_hoseLength{ 0.f };
		int m_maxRange{ 4 };
		int m_currentRange{ 0 };
		float m_extendSpeed{ 12.f };
		float m_retractSpeed{ 16.f };

		float m_retractTimer{ 0.f };
		static constexpr float RetractDelay{ 0.15f };

		// Hose sprite data — source rects on the spritesheet
		struct HoseSprite
		{
			SDL_FRect srcRect;    // Source rectangle on spritesheet
			float renderWidth;    // Rendered width in pixels (scaled to grid)
			float renderHeight;   // Rendered height in pixels
		};

		HoseSprite m_hoseRight;
		HoseSprite m_hoseLeft;
		HoseSprite m_hoseUp;
		HoseSprite m_hoseDown;

		std::shared_ptr<Texture2D> m_spriteSheet;

		void CalculateRange();
		void CacheComponents();
	};
}