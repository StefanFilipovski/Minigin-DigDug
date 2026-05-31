#include "GameOverState.h"
#include "HighScoreState.h"
#include "MenuState.h"
#include "GameStateManager.h"
#include "GameSession.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "TextComponent.h"
#include "ServiceLocator.h"
#include "GameSounds.h"
#include "Command.h"
#include <SDL3/SDL.h>
#include <string>

namespace dae
{
	class GameOverContinueCommand final : public Command
	{
	public:
		explicit GameOverContinueCommand(GameOverState* pState) : m_pState(pState) {}
		void Execute() override { m_pState->Continue(); }
	private:
		GameOverState* m_pState;
	};

	void GameOverState::OnEnter()
	{
		m_displayTimer = 0.f;
		m_canContinue = false;

		auto& sceneMgr = SceneManager::GetInstance();

		// Create or reuse game over scene
		m_pScene = sceneMgr.GetScene("GameOver");
		if (!m_pScene)
		{
			auto& scene = sceneMgr.CreateScene("GameOver");
			m_pScene = &scene;
		}
		else
		{
			m_pScene->RemoveAll();
		}

		sceneMgr.SetActiveScene("GameOver");

		// Silence any lingering music/jingles from gameplay before the fanfare
		ServiceLocator::GetSoundService().StopMusic();
		ServiceLocator::GetSoundService().StopAllSounds();
		ServiceLocator::GetSoundService().PlaySound(Sounds::GameOver);

		auto& session = GameSession::GetInstance();
		auto fontLarge = ResourceManager::GetInstance().LoadFont("Lingua.otf", 48);
		auto fontMed = ResourceManager::GetInstance().LoadFont("Lingua.otf", 28);
		auto fontSmall = ResourceManager::GetInstance().LoadFont("Lingua.otf", 20);

		const bool isVersus = (session.GetGameMode() == GameMode::Versus);

		// Title — in Versus, announce the winner instead of "GAME OVER"
		std::string titleText = "GAME OVER";
		if (isVersus)
		{
			titleText = (session.GetVersusWinner() == VersusWinner::Player)
				? "DIG DUG WINS!" : "FYGAR WINS!";
		}

		auto title = std::make_unique<GameObject>();
		title->AddComponent<TransformComponent>()->SetLocalPosition(250.f, 100.f);
		title->AddComponent<TextComponent>(fontLarge, titleText);
		m_pScene->Add(std::move(title));

		if (!isVersus)
		{
			// Player 1 score
			auto p1Score = std::make_unique<GameObject>();
			p1Score->AddComponent<TransformComponent>()->SetLocalPosition(280.f, 220.f);
			p1Score->AddComponent<TextComponent>(fontMed,
				"Player 1: " + std::to_string(session.GetPlayer1Score()));
			m_pScene->Add(std::move(p1Score));

			// Player 2 score (co-op only)
			if (session.GetGameMode() == GameMode::CoOp)
			{
				auto p2Score = std::make_unique<GameObject>();
				p2Score->AddComponent<TransformComponent>()->SetLocalPosition(280.f, 270.f);
				p2Score->AddComponent<TextComponent>(fontMed,
					"Player 2: " + std::to_string(session.GetPlayer2Score()));
				m_pScene->Add(std::move(p2Score));
			}

			// Round reached
			auto roundText = std::make_unique<GameObject>();
			roundText->AddComponent<TransformComponent>()->SetLocalPosition(300.f, 340.f);
			roundText->AddComponent<TextComponent>(fontSmall,
				"Round reached: " + std::to_string(session.GetCurrentRound()));
			m_pScene->Add(std::move(roundText));
		}

		// Continue prompt
		auto prompt = std::make_unique<GameObject>();
		prompt->AddComponent<TransformComponent>()->SetLocalPosition(250.f, 440.f);
		prompt->AddComponent<TextComponent>(fontSmall, "Press Enter or A to continue...");
		m_pScene->Add(std::move(prompt));

		BindInput();
	}

	void GameOverState::OnExit()
	{
		// Input clearing is handled by GameStateManager::SetState
	}

	void GameOverState::Update(float deltaTime)
	{
		m_displayTimer += deltaTime;
		if (m_displayTimer >= MinDisplayTime)
			m_canContinue = true;
	}

	void GameOverState::FixedUpdate(float fixedTimeStep)
	{
		(void)fixedTimeStep;
	}

	void GameOverState::Render() const
	{
	}

	void GameOverState::BindInput()
	{
		auto& input = InputManager::GetInstance();
		input.ClearAllBindings();

		input.BindKeyboardCommand(SDL_SCANCODE_RETURN, KeyState::Down,
			std::make_unique<GameOverContinueCommand>(this));
		input.BindKeyboardCommand(SDL_SCANCODE_SPACE, KeyState::Down,
			std::make_unique<GameOverContinueCommand>(this));

		input.BindControllerCommand(0, Controller::Button::A, KeyState::Down,
			std::make_unique<GameOverContinueCommand>(this));
	}

	void GameOverState::Continue()
	{
		if (!m_canContinue) return;

		auto mode = GameSession::GetInstance().GetGameMode();
		if (mode == GameMode::SinglePlayer)
		{
			// Single player: enter name for high score
			GameStateManager::GetInstance().SetState<HighScoreState>();
		}
		else
		{
			// Co-op / Versus: skip high score, go straight to menu
			GameSession::GetInstance().ResetForNewGame();
			GameStateManager::GetInstance().SetState<MenuState>();
		}
	}
}