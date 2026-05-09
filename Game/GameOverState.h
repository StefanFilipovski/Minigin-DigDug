#pragma once
#include "GameState.h"

namespace dae
{
	class Scene;

	class GameOverState final : public GameState
	{
	public:
		GameOverState() = default;

		void OnEnter() override;
		void OnExit() override;
		void Update(float deltaTime) override;
		void FixedUpdate(float fixedTimeStep) override;
		void Render() const override;
		void Continue();

	private:
		void BindInput();
		

		Scene* m_pScene{ nullptr };
		float m_displayTimer{ 0.f };
		static constexpr float MinDisplayTime{ 2.f }; // Minimum seconds before input accepted
		bool m_canContinue{ false };
	};
}