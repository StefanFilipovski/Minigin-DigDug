#include <algorithm>
#include "Scene.h"
#include <assert.h>

using namespace dae;

void Scene::Add(std::unique_ptr<GameObject> object)
{
	assert(object != nullptr);
	m_Objects.emplace_back(std::move(object));
}

void Scene::Remove(const GameObject& object)
{
	auto it = std::find_if(m_Objects.begin(), m_Objects.end(),
		[&object](const auto& ptr) { return ptr.get() == &object; });

	if (it != m_Objects.end())
		(*it)->MarkForDestroy();
}

void Scene::RemoveAll()
{
	m_Objects.clear();
}

void Scene::Update(float deltaTime)
{
	// Update objects regardless of hierarchy
		for (auto& object : m_Objects)
		object->Update(deltaTime);

	// Clean up dead objects
	m_Objects.erase(
		std::remove_if(m_Objects.begin(), m_Objects.end(),
			[](const auto& ptr) { return ptr->IsMarkedForDestroy(); }),
		m_Objects.end());
}

void Scene::FixedUpdate(float fixedTimeStep)
{
	for (auto& object : m_Objects)
		object->FixedUpdate(fixedTimeStep);
}

void Scene::Render() const
{
	// Render objects
	for (const auto& object : m_Objects)
		object->Render();
}