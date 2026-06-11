#include <algorithm>
#include "Scene.h"
#include <assert.h>

using namespace dae;

void Scene::Add(std::unique_ptr<GameObject> object)
{
	assert(object != nullptr);
	if (m_isUpdating)
		m_PendingAdds.emplace_back(std::move(object));
	else
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
	// Clean up objects that were marked for destruction in the previous frame.
	// Deferring by one frame lets game-state code (which runs before Scene::Update)
	// safely call IsMarkedForDestroy() to prune its own raw pointers before
	// the objects are actually freed.
	m_Objects.erase(
		std::remove_if(m_Objects.begin(), m_Objects.end(),
			[](const auto& ptr) { return ptr->IsMarkedForDestroy(); }),
		m_Objects.end());

	// Update surviving objects — additions during this loop are buffered
	// so that m_Objects isn't reallocated while we iterate it.
	m_isUpdating = true;
	for (auto& object : m_Objects)
		object->Update(deltaTime);
	m_isUpdating = false;

	// Flush any objects that were added during the update
	for (auto& pending : m_PendingAdds)
		m_Objects.emplace_back(std::move(pending));
	m_PendingAdds.clear();
}

void Scene::FixedUpdate(float fixedTimeStep)
{
	// Buffer additions here too — components can spawn objects during FixedUpdate
	m_isUpdating = true;
	for (auto& object : m_Objects)
		object->FixedUpdate(fixedTimeStep);
	m_isUpdating = false;

	for (auto& pending : m_PendingAdds)
		m_Objects.emplace_back(std::move(pending));
	m_PendingAdds.clear();
}

void Scene::Render() const
{
	// Render objects
	for (const auto& object : m_Objects)
		object->Render();
}

void Scene::RenderImGui() const
{
	for (const auto& object : m_Objects)
		object->RenderImGui();
}