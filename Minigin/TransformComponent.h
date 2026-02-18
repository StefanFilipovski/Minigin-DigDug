#pragma once
#include "Component.h"
#include "Transform.h"

namespace dae
{
	class TransformComponent final : public Component
	{
	public:
		explicit TransformComponent(GameObject* pOwner) : Component(pOwner) {}

		const glm::vec3& GetPosition() const { return m_Transform.GetPosition(); }
		void SetPosition(float x, float y, float z = 0) { m_Transform.SetPosition(x, y, z); }

	private:
		Transform m_Transform{};
	};
}