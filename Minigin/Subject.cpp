#include "Subject.h"
#include <algorithm>

namespace dae
{
	void Subject::AddObserver(Observer* pObserver)
	{
		m_Observers.push_back(pObserver);
	}

	void Subject::RemoveObserver(Observer* pObserver)
	{
		m_Observers.erase(
			std::remove(m_Observers.begin(), m_Observers.end(), pObserver),
			m_Observers.end());
	}

	void Subject::NotifyObservers(EventId event, GameObject* pGameObject)
	{
		// iterate over a copy in case observers modify the list during notification
		auto observers = m_Observers;
		for (auto* pObserver : observers)
			pObserver->Notify(event, pGameObject);
	}
}