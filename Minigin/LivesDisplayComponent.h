#pragma once
#include "Component.h"
#include "Observer.h"

namespace dae
{
	class TextComponent;

	class LivesDisplayComponent final : public Component, public Observer
	{
	public:
		explicit LivesDisplayComponent(GameObject* pOwner);

		void Notify(EventId event, GameObject* pGameObject) override;
	};
}