#pragma once
#include "Component.h"
#include "Observer.h"
#include <string>

namespace dae
{
	class PointsDisplayComponent final : public Component, public Observer
	{
	public:
		explicit PointsDisplayComponent(GameObject* pOwner, const std::string& prefix = "Score: ");

		void Notify(EventId event, GameObject* pGameObject) override;

	private:
		std::string m_prefix;
	};
}