#include "Subject.h"
#include <algorithm>

namespace dae
{
	Observer::~Observer()
	{
		// Copy: RemoveObserver erases from m_Subjects via the backlink below
		auto subjects = m_Subjects;
		for (auto* pSubject : subjects)
			pSubject->RemoveObserver(this);
	}

	Subject::~Subject()
	{
		for (auto* pObserver : m_Observers)
		{
			if (!pObserver) continue;
			auto& subjects = pObserver->m_Subjects;
			subjects.erase(
				std::remove(subjects.begin(), subjects.end(), this),
				subjects.end());
		}
	}

	void Subject::AddObserver(Observer* pObserver)
	{
		if (!pObserver) return;
		m_Observers.push_back(pObserver);
		pObserver->m_Subjects.push_back(this);
	}

	void Subject::RemoveObserver(Observer* pObserver)
	{
		if (!pObserver) return;

		if (m_notifyDepth > 0)
		{
			// Mid-notification: null the slot so the iteration in
			// NotifyObservers stays valid; compact afterwards
			for (auto& slot : m_Observers)
			{
				if (slot == pObserver)
				{
					slot = nullptr;
					m_needsCompact = true;
				}
			}
		}
		else
		{
			m_Observers.erase(
				std::remove(m_Observers.begin(), m_Observers.end(), pObserver),
				m_Observers.end());
		}

		auto& subjects = pObserver->m_Subjects;
		subjects.erase(
			std::remove(subjects.begin(), subjects.end(), this),
			subjects.end());
	}

	void Subject::NotifyObservers(EventId event, GameObject* pGameObject)
	{
		
		++m_notifyDepth;
		const size_t count = m_Observers.size();
		for (size_t i = 0; i < count; ++i)
		{
			if (m_Observers[i])
				m_Observers[i]->Notify(event, pGameObject);
		}
		--m_notifyDepth;

		if (m_notifyDepth == 0 && m_needsCompact)
		{
			m_Observers.erase(
				std::remove(m_Observers.begin(), m_Observers.end(), nullptr),
				m_Observers.end());
			m_needsCompact = false;
		}
	}
}
