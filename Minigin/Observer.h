#pragma once
#include "EventIds.h"

namespace dae
{
	class GameObject;

	class Observer
	{
	public:
		virtual ~Observer() = default;
		virtual void Notify(EventId event, GameObject* pGameObject) = 0;
	};
}