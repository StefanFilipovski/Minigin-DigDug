#pragma once
#include "Observer.h"
#include <vector>

namespace dae
{
	class GameObject;

	class Subject
	{
	public:
		Subject() = default;
		// Clears the backlink in every registered observer
		virtual ~Subject();

		Subject(const Subject&) = delete;
		Subject(Subject&&) = delete;
		Subject& operator=(const Subject&) = delete;
		Subject& operator=(Subject&&) = delete;

		void AddObserver(Observer* pObserver);
		void RemoveObserver(Observer* pObserver);

	protected:
		void NotifyObservers(EventId event, GameObject* pGameObject);

	private:
		std::vector<Observer*> m_Observers{};

		
		int m_notifyDepth{ 0 };
		bool m_needsCompact{ false };
	};
}
