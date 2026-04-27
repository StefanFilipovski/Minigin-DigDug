#pragma once
#include "GameState.h"
#include "GameMode.h"

namespace dae
{
	class Scene;

	class MenuState final : public GameState
	{
	public:
		MenuState() = default;

		void OnEnter() override;
		void OnExit() override;
		void Update(float deltaTime) override;
		void FixedUpdate(float fixedTimeStep) override;
		void Render() const override;

		GameMode GetSelectedMode() const { return m_selectedMode; }
		void SelectMode(int direction);   // -1 = up, +1 = down
		void ConfirmSelection();
	private:
		void BindInput();
		void UpdateSelectionVisual();

		GameMode m_selectedMode{ GameMode::SinglePlayer };
		int m_selectedIndex{ 0 };

		Scene* m_pScene{ nullptr };

		// Raw pointers to UI GameObjects for updating visuals
		class GameObject* m_pSelectorIndicator{ nullptr };
		class GameObject* m_pTitleText{ nullptr };
	};
}