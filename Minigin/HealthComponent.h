#pragma once
#include "Component.h"
#include "Subject.h"

namespace dae
{
	class HealthComponent final : public Component, public Subject
	{
	public:
		explicit HealthComponent(GameObject* pOwner, int lives = 3);

		void Die();

		int GetLives() const { return m_Lives; }
		bool IsDead()  const { return m_Lives <= 0; }

	private:
		int m_Lives;
	};
}