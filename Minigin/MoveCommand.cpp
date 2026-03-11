#include "MoveCommand.h"
#include "GameObject.h"
#include "TransformComponent.h"

namespace dae
{
	MoveCommand::MoveCommand(GameObject* pGameObject, const glm::vec3& direction, float speed)
		: GameActorCommand(pGameObject)
		, m_Direction(direction)
		, m_Speed(speed)
	{
	}

	void MoveCommand::Execute()
	{
		auto* pActor = GetGameActor();
		if (!pActor) return;

		auto* pTransform = pActor->GetComponent<TransformComponent>();
		if (!pTransform) return;

		const glm::vec3 newPos = pTransform->GetLocalPosition() + m_Direction * m_Speed;
		pTransform->SetLocalPosition(newPos);
	}
}