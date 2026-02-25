#include "TransformComponent.h"
#include "GameObject.h"

namespace dae
{
	void TransformComponent::SetLocalPosition(float x, float y, float z)
	{
		m_localPosition = { x, y, z };
		SetPositionDirty();
	}

	void TransformComponent::SetLocalPosition(const glm::vec3& pos)
	{
		m_localPosition = pos;
		SetPositionDirty();
	}

	// Mark this node and all children as dirty (propagate down the hierarchy)
	void TransformComponent::SetPositionDirty()
	{
		m_positionIsDirty = true;

		// Propagate dirtiness to all children
		GameObject* owner = GetOwner();
		for (int i = 0; i < owner->GetChildCount(); ++i)
		{
			auto* childTransform = owner->GetChildAt(i)->GetComponent<TransformComponent>();
			if (childTransform)
				childTransform->SetPositionDirty();
		}
	}

	const glm::vec3& TransformComponent::GetWorldPosition()
	{
		if (m_positionIsDirty)
			UpdateWorldPosition();
		return m_worldPosition;
	}

	void TransformComponent::UpdateWorldPosition()
	{
		GameObject* parent = GetOwner()->GetParent();
		if (parent == nullptr)
		{
			m_worldPosition = m_localPosition;
		}
		else
		{
			auto* parentTransform = parent->GetComponent<TransformComponent>();
			if (parentTransform)
				m_worldPosition = parentTransform->GetWorldPosition() + m_localPosition;
			else
				m_worldPosition = m_localPosition;
		}
		m_positionIsDirty = false;
	}
}