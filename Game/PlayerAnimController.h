#pragma once
#include "Component.h"

namespace dae
{
	class GridMovementComponent;
	class SpriteAnimatorComponent;

	// Watches movement direction and plays the correct walk/dig/idle animation.
	// Attach to the same GameObject as GridMovementComponent and SpriteAnimatorComponent.
	class PlayerAnimController final : public Component
	{
	public:
		explicit PlayerAnimController(GameObject* pOwner) : Component(pOwner) {}

		void Update(float deltaTime) override;

	private:
		GridMovementComponent* m_pMovement{ nullptr };
		SpriteAnimatorComponent* m_pAnimator{ nullptr };
		bool m_cached{ false };
	};
}