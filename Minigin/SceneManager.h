#pragma once
#include <vector>
#include <string>
#include <memory>
#include "Scene.h"
#include "Singleton.h"

namespace dae
{
	class SceneManager final : public Singleton<SceneManager>
	{
	public:
		Scene& CreateScene(const std::string& name = "");

		// Active scene management — only the active scene is updated/rendered
		void SetActiveScene(const std::string& name);
		void SetActiveScene(size_t index);
		Scene* GetActiveScene() const;
		Scene* GetScene(const std::string& name) const;

		// Remove a scene by name (cannot remove the active scene)
		void RemoveScene(const std::string& name);

		void Update(float deltaTime);
		void FixedUpdate(float fixedTimeStep);
		void Render();
		void RenderImGui();

	private:
		friend class Singleton<SceneManager>;
		SceneManager() = default;

		struct SceneEntry
		{
			std::string name;
			std::unique_ptr<Scene> scene;
		};

		std::vector<SceneEntry> m_scenes{};
		size_t m_activeSceneIndex{ 0 };
	};
}