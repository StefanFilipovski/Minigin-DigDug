#pragma once
#include "EventIds.h"
#include <vector>

namespace dae
{
	class GameObject;
	class Subject;

	class Observer
	{
	public:
		Observer() = default;
		// Auto-unregisters from every subject still watching us,
		// so a destroyed observer can never leave a dangling pointer behind
		virtual ~Observer();

		Observer(const Observer&) = delete;
		Observer(Observer&&) = delete;
		Observer& operator=(const Observer&) = delete;
		Observer& operator=(Observer&&) = delete;

		virtual void Notify(EventId event, GameObject* pGameObject) = 0;

	private:
		friend class Subject;
		std::vector<Subject*> m_Subjects{};
	};
}
