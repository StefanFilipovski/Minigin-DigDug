#pragma once
#include "Component.h"

namespace dae
{
	class GridMovementComponent;
	class SpriteAnimatorComponent;
	class PlayerCollisionComponent;
	class PumpComponent;

	
	class PlayerAnimController final : public Component
	{
	public:
		explicit PlayerAnimController(GameObject* pOwner) : Component(pOwner) {}

		void Update(float deltaTime) override;

	private:
		GridMovementComponent* m_pMovement{ nullptr };
		SpriteAnimatorComponent* m_pAnimator{ nullptr };
		PlayerCollisionComponent* m_pCollision{ nullptr };
		PumpComponent* m_pPump{ nullptr };
		bool m_cached{ false };
	};
}