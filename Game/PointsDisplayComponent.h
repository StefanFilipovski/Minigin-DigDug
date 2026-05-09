#pragma once
#include "Component.h"
#include "Observer.h"

namespace dae
{
	class PointsDisplayComponent final : public Component, public Observer
	{
	public:
		explicit PointsDisplayComponent(GameObject* pOwner);

		void Notify(EventId event, GameObject* pGameObject) override;
	};
}