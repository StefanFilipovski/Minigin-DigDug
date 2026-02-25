#include "GameObject.h"
#include "TransformComponent.h"
#include "Component.h"
#include <algorithm>
#include <cassert>

namespace dae
{
	GameObject::~GameObject()
	{
		// Detach all children so they dont hold a dangling m_parent pointer
		for (auto* child : m_children)
			child->m_parent = nullptr;

		// Remove ourselves from our parents children list
		if (m_parent)
			m_parent->RemoveChild(this);
	}

	void GameObject::Update(float deltaTime)
	{
		for (auto& c : m_Components)
			c->Update(deltaTime);

		m_Components.erase(
			std::remove_if(m_Components.begin(), m_Components.end(),
				[](const auto& c) { return c->IsMarkedForDestroy(); }),
			m_Components.end());
	}

	void GameObject::FixedUpdate(float fixedTimeStep)
	{
		for (auto& c : m_Components)
			c->FixedUpdate(fixedTimeStep);
	}

	void GameObject::Render() const
	{
		for (const auto& c : m_Components)
			c->Render();
	}

	void GameObject::SetParent(GameObject* parent, bool keepWorldPosition)
	{
		
		if (IsDescendant(parent) || parent == this || m_parent == parent)
			return;

	
		auto* myTransform = GetComponent<TransformComponent>();
		if (myTransform)
		{
			if (parent == nullptr)
			{
				myTransform->SetLocalPosition(myTransform->GetWorldPosition());
			}
			else
			{
				if (keepWorldPosition)
				{
					auto* parentTransform = parent->GetComponent<TransformComponent>();
					const glm::vec3 parentWorld = parentTransform
						? parentTransform->GetWorldPosition()
						: glm::vec3{};
					myTransform->SetLocalPosition(myTransform->GetWorldPosition() - parentWorld);
				}
				myTransform->SetPositionDirty();
			}
		}

		// remove from old parent
		if (m_parent)
			m_parent->RemoveChild(this);

		// set new parent
		m_parent = parent;

		// add to new parent
		if (m_parent)
			m_parent->AddChild(this);
	}

	void GameObject::AddChild(GameObject* child)
	{
		assert(child != nullptr);
		assert(std::find(m_children.begin(), m_children.end(), child) == m_children.end());
		m_children.emplace_back(child);
	}

	void GameObject::RemoveChild(GameObject* child)
	{
		auto it = std::find(m_children.begin(), m_children.end(), child);
		if (it != m_children.end())
			m_children.erase(it);
	}

	bool GameObject::IsDescendant(const GameObject* candidate) const
	{
		for (const auto* child : m_children)
		{
			if (child == candidate)
				return true;
			if (child->IsDescendant(candidate))
				return true;
		}
		return false;
	}

} // namespace dae