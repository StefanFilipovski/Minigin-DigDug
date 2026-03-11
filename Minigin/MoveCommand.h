#pragma once
#include "Command.h"
#include <glm/glm.hpp>


namespace dae
{
	class MoveCommand final : public GameActorCommand
	{
	public:
		MoveCommand(GameObject* pGameObject, const glm::vec3& direction, float speed);
		void Execute() override;

	private:
		glm::vec3 m_Direction;
		float     m_Speed;
	};
}