#include "MenuState.h"
#include "GameStateManager.h"
#include "GameSession.h"
#include "PlayingState.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "RenderComponent.h"
#include "TextComponent.h"
#include "Command.h"
#include <SDL3/SDL.h>

namespace dae
{
	// ---- Menu-specific commands ----
	class MenuNavigateCommand final : public Command
	{
	public:
		MenuNavigateCommand(MenuState* pMenu, int direction)
			: m_pMenu(pMenu), m_direction(direction) {
		}
		void Execute() override { m_pMenu->SelectMode(m_direction); }
	private:
		MenuState* m_pMenu;
		int m_direction;
	};

	class MenuConfirmCommand final : public Command
	{
	public:
		explicit MenuConfirmCommand(MenuState* pMenu) : m_pMenu(pMenu) {}
		void Execute() override { m_pMenu->ConfirmSelection(); }
	private:
		MenuState* m_pMenu;
	};

	// ---- MenuState implementation ----
	void MenuState::OnEnter()
	{
		auto& sceneMgr = SceneManager::GetInstance();

		// Create the menu scene if it doesn't exist yet
		m_pScene = sceneMgr.GetScene("Menu");
		if (!m_pScene)
		{
			auto& scene = sceneMgr.CreateScene("Menu");
			m_pScene = &scene;

			auto font = ResourceManager::GetInstance().LoadFont("Lingua.otf", 36);
			auto fontSmall = ResourceManager::GetInstance().LoadFont("Lingua.otf", 24);

			// Title
			auto title = std::make_unique<GameObject>();
			title->AddComponent<TransformComponent>()->SetLocalPosition(250.f, 80.f);
			title->AddComponent<TextComponent>(font, "DIG DUG");
			m_pTitleText = title.get();
			scene.Add(std::move(title));

			// Mode options
			constexpr float startY = 220.f;
			constexpr float spacing = 50.f;
			const char* labels[] = { "Single Player", "Co-Op", "Versus" };

			for (int i = 0; i < 3; ++i)
			{
				auto option = std::make_unique<GameObject>();
				option->AddComponent<TransformComponent>()->SetLocalPosition(300.f, startY + i * spacing);
				option->AddComponent<TextComponent>(fontSmall, labels[i]);
				scene.Add(std::move(option));
			}

			// Selection indicator (a ">" character)
			auto selector = std::make_unique<GameObject>();
			selector->AddComponent<TransformComponent>()->SetLocalPosition(270.f, startY);
			selector->AddComponent<TextComponent>(fontSmall, ">");
			m_pSelectorIndicator = selector.get();
			scene.Add(std::move(selector));

			// Controls hint
			auto hint = std::make_unique<GameObject>();
			hint->AddComponent<TransformComponent>()->SetLocalPosition(180.f, 450.f);
			auto fontHint = ResourceManager::GetInstance().LoadFont("Lingua.otf", 16);
			hint->AddComponent<TextComponent>(fontHint, "W/S or DPad to select  -  Enter/A to confirm");
			scene.Add(std::move(hint));
		}

		sceneMgr.SetActiveScene("Menu");
		m_selectedIndex = 0;
		m_selectedMode = GameMode::SinglePlayer;
		UpdateSelectionVisual();
		BindInput();
	}

	void MenuState::OnExit()
	{
		// Input clearing is handled by GameStateManager::SetState
	}

	void MenuState::Update(float deltaTime)
	{
		// Scene update is handled by SceneManager in the game loop
		(void)deltaTime;
	}

	void MenuState::FixedUpdate(float fixedTimeStep)
	{
		(void)fixedTimeStep;
	}

	void MenuState::Render() const
	{
		// Scene render is handled by SceneManager in the game loop
	}

	void MenuState::BindInput()
	{
		auto& input = InputManager::GetInstance();
		input.ClearAllBindings();

		// Keyboard
		input.BindKeyboardCommand(SDL_SCANCODE_W, KeyState::Down,
			std::make_unique<MenuNavigateCommand>(this, -1));
		input.BindKeyboardCommand(SDL_SCANCODE_S, KeyState::Down,
			std::make_unique<MenuNavigateCommand>(this, 1));
		input.BindKeyboardCommand(SDL_SCANCODE_UP, KeyState::Down,
			std::make_unique<MenuNavigateCommand>(this, -1));
		input.BindKeyboardCommand(SDL_SCANCODE_DOWN, KeyState::Down,
			std::make_unique<MenuNavigateCommand>(this, 1));
		input.BindKeyboardCommand(SDL_SCANCODE_RETURN, KeyState::Down,
			std::make_unique<MenuConfirmCommand>(this));
		input.BindKeyboardCommand(SDL_SCANCODE_SPACE, KeyState::Down,
			std::make_unique<MenuConfirmCommand>(this));

		// Controller
		input.BindControllerCommand(0, Controller::Button::DPadUp, KeyState::Down,
			std::make_unique<MenuNavigateCommand>(this, -1));
		input.BindControllerCommand(0, Controller::Button::DPadDown, KeyState::Down,
			std::make_unique<MenuNavigateCommand>(this, 1));
		input.BindControllerCommand(0, Controller::Button::A, KeyState::Down,
			std::make_unique<MenuConfirmCommand>(this));
	}

	void MenuState::SelectMode(int direction)
	{
		m_selectedIndex = (m_selectedIndex + direction + 3) % 3;
		m_selectedMode = static_cast<GameMode>(m_selectedIndex);
		UpdateSelectionVisual();
	}

	void MenuState::ConfirmSelection()
	{
		auto& gsm = GameStateManager::GetInstance();

		// We need to find the registered PlayingState and set its mode
		// before the transition happens. We do this by accessing it
		// through the state manager's map. To keep it clean, we store
		// the mode and let PlayingState query us.
		// However, the simplest approach: get the state pointer we stored at registration.

		// The cleanest approach: PlayingState has a SetGameMode we call first.
		// We access it via a static/global or via GameStateManager.
		// Since GameStateManager stores states by type, we add a GetState<T>() helper.
		// For now, we use a simpler approach with a shared GameMode variable.

		// Using the GameSession approach:
		GameSession::GetInstance().SetGameMode(m_selectedMode);
		gsm.SetState<PlayingState>();
	}

	void MenuState::UpdateSelectionVisual()
	{
		if (!m_pSelectorIndicator) return;

		constexpr float startY = 220.f;
		constexpr float spacing = 50.f;
		auto* transform = m_pSelectorIndicator->GetComponent<TransformComponent>();
		if (transform)
			transform->SetLocalPosition(270.f, startY + m_selectedIndex * spacing);
	}
}