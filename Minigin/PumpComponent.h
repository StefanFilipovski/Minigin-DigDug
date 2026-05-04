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

		// PumpString.png is a 32×6 horizontal sprite (extending right).
		// We rotate it for up/down/left and clip the source to animate extension.
		std::shared_ptr<Texture2D> m_hoseTexture;
		float m_hoseSrcW{ 32.f }; // native sprite width
		float m_hoseSrcH{ 6.f };  // native sprite height (the "thickness" of the rope)
		float m_hoseRenderLong{ 32.f };  // pixels along the firing axis at full extension
		float m_hoseRenderShort{ 6.f };  // pixels perpendicular to firing axis

		void CalculateRange();
		void CacheComponents();
	};
}