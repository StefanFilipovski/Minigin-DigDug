#include "GameObject.h"
#include "Component.h"
#include <algorithm>

void dae::GameObject::Update(float deltaTime)
{
	for (auto& c : m_Components)
		c->Update(deltaTime);

	m_Components.erase(
		std::remove_if(m_Components.begin(), m_Components.end(),
			[](const auto& c) { return c->IsMarkedForDestroy(); }),
		m_Components.end()
	);
}

void dae::GameObject::FixedUpdate(float fixedTimeStep)
{
	for (auto& c : m_Components)
		c->FixedUpdate(fixedTimeStep);
}

void dae::GameObject::Render() const
{
	for (const auto& c : m_Components)
		c->Render();
}