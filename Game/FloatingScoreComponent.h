#pragma once
#include "Component.h"

namespace dae
{
	class TextComponent;
	class TransformComponent;

	// Spawns as blue score text, floats upward, then self-destructs.
	class FloatingScoreComponent final : public Component
	{
	public:
		FloatingScoreComponent(GameObject* pOwner, float lifetime = 1.0f, float riseSpeed = 50.f);

		void Update(float deltaTime) override;

	private:
		TransformComponent* m_pTransform{ nullptr };
		bool m_cached{ false };

		float m_lifetime;
		float m_riseSpeed;
		float m_timer{ 0.f };
	};
}
