#pragma once
#include "Component.h"

namespace dae
{
	class TextComponent;
	class TransformComponent;

	// Blue score text that floats upward. Pooled: starts inactive, returns
	// to the pool (deactivates) when its lifetime runs out.
	class FloatingScoreComponent final : public Component
	{
	public:
		FloatingScoreComponent(GameObject* pOwner, float lifetime = 1.0f, float riseSpeed = 50.f);

		void Update(float deltaTime) override;

		void Activate();
		bool IsActive() const { return m_active; }

	private:
		TransformComponent* m_pTransform{ nullptr };
		bool m_cached{ false };
		bool m_active{ false };

		float m_lifetime;
		float m_riseSpeed;
		float m_timer{ 0.f };
	};
}
