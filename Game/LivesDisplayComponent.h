#pragma once
#include "Component.h"
#include "Observer.h"
#include <string>

namespace dae
{
	class TextComponent;

	class LivesDisplayComponent final : public Component, public Observer
	{
	public:
		explicit LivesDisplayComponent(GameObject* pOwner, const std::string& prefix = "Lives: ");

		void Notify(EventId event, GameObject* pGameObject) override;

	private:
		std::string m_prefix;
	};
}