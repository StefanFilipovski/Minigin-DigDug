#pragma once
#include "Component.h"
#include "Subject.h"

namespace dae
{
	class ScoreComponent final : public Component, public Subject
	{
	public:
		explicit ScoreComponent(GameObject* pOwner);

		void AddPoints(int points);
		int  GetScore() const { return m_Score; }

	private:
		int m_Score{ 0 };
	};
}