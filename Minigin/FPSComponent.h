#pragma once
#include "Component.h"

namespace dae
{
	class FPSComponent final : public Component
	{
	public:
		explicit FPSComponent(GameObject* pOwner) : Component(pOwner) {}

		void Update(float deltaTime) override;

	private:
		float m_AccumulatedTime{};
		int   m_FrameCount{};
	};
}