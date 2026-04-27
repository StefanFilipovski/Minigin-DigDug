#include "SceneManager.h"
#include "Scene.h"
#include <algorithm>
#include <cassert>

namespace dae
{
	Scene& SceneManager::CreateScene(const std::string& name)
	{
		auto scene = std::unique_ptr<Scene>(new Scene());
		Scene* ptr = scene.get();
		m_scenes.push_back({ name, std::move(scene) });
		return *ptr;
	}

	void SceneManager::SetActiveScene(const std::string& name)
	{
		for (size_t i = 0; i < m_scenes.size(); ++i)
		{
			if (m_scenes[i].name == name)
			{
				m_activeSceneIndex = i;
				return;
			}
		}
		assert(false && "Scene not found by name");
	}

	void SceneManager::SetActiveScene(size_t index)
	{
		assert(index < m_scenes.size() && "Scene index out of range");
		m_activeSceneIndex = index;
	}

	Scene* SceneManager::GetActiveScene() const
	{
		if (m_scenes.empty()) return nullptr;
		return m_scenes[m_activeSceneIndex].scene.get();
	}

	Scene* SceneManager::GetScene(const std::string& name) const
	{
		for (const auto& entry : m_scenes)
		{
			if (entry.name == name)
				return entry.scene.get();
		}
		return nullptr;
	}

	void SceneManager::RemoveScene(const std::string& name)
	{
		auto it = std::find_if(m_scenes.begin(), m_scenes.end(),
			[&name](const SceneEntry& entry) { return entry.name == name; });

		if (it == m_scenes.end()) return;

		size_t removedIndex = static_cast<size_t>(std::distance(m_scenes.begin(), it));
		assert(removedIndex != m_activeSceneIndex && "Cannot remove the active scene");

		m_scenes.erase(it);

		// Adjust active index if needed
		if (m_activeSceneIndex > removedIndex && m_activeSceneIndex > 0)
			--m_activeSceneIndex;
	}

	void SceneManager::Update(float deltaTime)
	{
		if (auto* scene = GetActiveScene())
			scene->Update(deltaTime);
	}

	void SceneManager::FixedUpdate(float fixedTimeStep)
	{
		if (auto* scene = GetActiveScene())
			scene->FixedUpdate(fixedTimeStep);
	}

	void SceneManager::Render()
	{
		if (auto* scene = GetActiveScene())
			scene->Render();
	}

	void SceneManager::RenderImGui()
	{
		if (auto* scene = GetActiveScene())
			scene->RenderImGui();
	}
}