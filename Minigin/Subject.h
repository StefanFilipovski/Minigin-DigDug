#pragma once
#include "Observer.h"
#include <vector>

namespace dae
{
	class GameObject;

	class Subject
	{
	public:
		virtual ~Subject() = default;

		void AddObserver(Observer* pObserver);
		void RemoveObserver(Observer* pObserver);

	protected:
		void NotifyObservers(EventId event, GameObject* pGameObject);

	private:
		std::vector<Observer*> m_Observers{};
	};
}