#pragma once
#include <vector>
#include <memory>
#include "Scene.h"
#include <Singleton.h>

namespace dae
{
	class SceneManager final : public Singleton<SceneManager>
	{
	public:
		Scene& CreateScene();

		void Update(float deltaTime);
		void FixedUpdate(float fixedTimeStep);
		void Render();

	private:
		friend class Singleton<SceneManager>;
		SceneManager() = default;

		std::vector<std::unique_ptr<Scene>> m_scenes{};
	};
}