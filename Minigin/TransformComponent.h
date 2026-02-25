#pragma once
#include "Component.h"
#include <glm/glm.hpp>

namespace dae
{
	
	class TransformComponent final : public Component
	{
	public:
		explicit TransformComponent(GameObject* pOwner) : Component(pOwner) {}

		const glm::vec3& GetWorldPosition();

		// Local position 
		const glm::vec3& GetLocalPosition() const { return m_localPosition; }

		void SetLocalPosition(float x, float y, float z = 0.f);
		void SetLocalPosition(const glm::vec3& pos);

		// helpers 
		void SetPosition(float x, float y, float z = 0.f) { SetLocalPosition(x, y, z); }
		const glm::vec3& GetPosition() { return GetWorldPosition(); }

		// Marks this transform and all descendants as needing recomputation.
		void SetPositionDirty();

	private:
		void UpdateWorldPosition();

		glm::vec3 m_localPosition{};
		glm::vec3 m_worldPosition{};

		// Starts true so the very first GetWorldPosition() always computes correctly
		bool m_positionIsDirty{ true };
	};
}