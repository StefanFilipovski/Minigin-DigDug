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
#include "Command.h"
#include <SDL3/SDL.h>
#include <string>
#include <sstream>
#include <algorithm>

namespace dae
{
	// ---- HighScore-specific commands ----
	class HSCycleCommand final : public Command
	{
	public:
		HSCycleCommand(HighScoreState* pState, int dir) : m_pState(pState), m_dir(dir) {}
		void Execute() override { m_pState->CycleCharacter(m_dir); }
	private:
		HighScoreState* m_pState;
		int m_dir;
	};

	class HSConfirmCommand final : public Command
	{
	public:
		explicit HSConfirmCommand(HighScoreState* pState) : m_pState(pState) {}
		void Execute() override { m_pState->ConfirmCharacter(); }
	private:
		HighScoreState* m_pState;
	};

	class HSReturnToMenuCommand final : public Command
	{
	public:
		void Execute() override
		{
			GameSession::GetInstance().ResetForNewGame();
			GameStateManager::GetInstance().SetState<MenuState>();
		}
	};

	// ---- HighScoreState implementation ----
	void HighScoreState::OnEnter()
	{
		m_currentCharIndex = 0;
		m_nameChars = { 'A', 'A', 'A' };
		m_entryComplete = false;

		auto& sceneMgr = SceneManager::GetInstance();
		m_pScene = sceneMgr.GetScene("HighScore");
		if (!m_pScene)
		{
			auto& scene = sceneMgr.CreateScene("HighScore");
			m_pScene = &scene;
		}
		else
		{
			m_pScene->RemoveAll();
		}

		sceneMgr.SetActiveScene("HighScore");

		// Load existing high scores
		GameSession::GetInstance().LoadHighScores(HighScoreFilePath);

		RebuildDisplay();
		BindInput();
	}

	void HighScoreState::OnExit()
	{
		// Input clearing is handled by GameStateManager::SetState
	}

	void HighScoreState::Update(float deltaTime)
	{
		(void)deltaTime;
	}

	void HighScoreState::FixedUpdate(float fixedTimeStep)
	{
		(void)fixedTimeStep;
	}

	void HighScoreState::Render() const
	{
	}

	void HighScoreState::CycleCharacter(int direction)
	{
		if (m_entryComplete) return;

		char& c = m_nameChars[m_currentCharIndex];
		c = static_cast<char>(c + direction);

		// Wrap around A-Z
		if (c > 'Z') c = 'A';
		if (c < 'A') c = 'Z';

		RebuildDisplay();
	}

	void HighScoreState::ConfirmCharacter()
	{
		if (m_entryComplete) return;

		++m_currentCharIndex;
		if (m_currentCharIndex >= NameLength)
		{
			FinishEntry();
			return;
		}

		RebuildDisplay();
	}

	void HighScoreState::FinishEntry()
	{
		m_entryComplete = true;

		// Build the 3-character name string
		std::string name(m_nameChars.begin(), m_nameChars.end());

		auto& session = GameSession::GetInstance();
		int score = session.GetPlayer1Score();

		// In co-op, use the higher score
		if (session.GetGameMode() == GameMode::CoOp)
			score = std::max(session.GetPlayer1Score(), session.GetPlayer2Score());

		session.AddHighScore(name, score);
		session.SaveHighScores(HighScoreFilePath);

		RebuildDisplay();

		// Rebind input: now only "return to menu" is available
		auto& input = InputManager::GetInstance();
		input.ClearAllBindings();

		input.BindKeyboardCommand(SDL_SCANCODE_RETURN, KeyState::Down,
			std::make_unique<HSReturnToMenuCommand>());
		input.BindKeyboardCommand(SDL_SCANCODE_SPACE, KeyState::Down,
			std::make_unique<HSReturnToMenuCommand>());
		input.BindControllerCommand(0, Controller::Button::A, KeyState::Down,
			std::make_unique<HSReturnToMenuCommand>());
	}

	void HighScoreState::BindInput()
	{
		auto& input = InputManager::GetInstance();
		input.ClearAllBindings();

		// W/S or DPad Up/Down to cycle letters
		input.BindKeyboardCommand(SDL_SCANCODE_W, KeyState::Down,
			std::make_unique<HSCycleCommand>(this, -1));
		input.BindKeyboardCommand(SDL_SCANCODE_S, KeyState::Down,
			std::make_unique<HSCycleCommand>(this, 1));
		input.BindKeyboardCommand(SDL_SCANCODE_UP, KeyState::Down,
			std::make_unique<HSCycleCommand>(this, -1));
		input.BindKeyboardCommand(SDL_SCANCODE_DOWN, KeyState::Down,
			std::make_unique<HSCycleCommand>(this, 1));

		// Enter/Space or A to confirm character
		input.BindKeyboardCommand(SDL_SCANCODE_RETURN, KeyState::Down,
			std::make_unique<HSConfirmCommand>(this));
		input.BindKeyboardCommand(SDL_SCANCODE_SPACE, KeyState::Down,
			std::make_unique<HSConfirmCommand>(this));

		// Controller
		input.BindControllerCommand(0, Controller::Button::DPadUp, KeyState::Down,
			std::make_unique<HSCycleCommand>(this, -1));
		input.BindControllerCommand(0, Controller::Button::DPadDown, KeyState::Down,
			std::make_unique<HSCycleCommand>(this, 1));
		input.BindControllerCommand(0, Controller::Button::A, KeyState::Down,
			std::make_unique<HSConfirmCommand>(this));
	}

	void HighScoreState::RebuildDisplay()
	{
		if (!m_pScene) return;
		m_pScene->RemoveAll();

		auto fontLarge = ResourceManager::GetInstance().LoadFont("Lingua.otf", 36);
		auto fontMed = ResourceManager::GetInstance().LoadFont("Lingua.otf", 28);
		auto fontSmall = ResourceManager::GetInstance().LoadFont("Lingua.otf", 20);

		// Title
		auto title = std::make_unique<GameObject>();
		title->AddComponent<TransformComponent>()->SetLocalPosition(260.f, 30.f);
		title->AddComponent<TextComponent>(fontLarge, "HIGH SCORES");
		m_pScene->Add(std::move(title));

		if (!m_entryComplete)
		{
			// Show name entry
			auto& session = GameSession::GetInstance();
			int score = session.GetPlayer1Score();

			auto scoreText = std::make_unique<GameObject>();
			scoreText->AddComponent<TransformComponent>()->SetLocalPosition(280.f, 90.f);
			scoreText->AddComponent<TextComponent>(fontMed,
				"Your score: " + std::to_string(score));
			m_pScene->Add(std::move(scoreText));

			auto prompt = std::make_unique<GameObject>();
			prompt->AddComponent<TransformComponent>()->SetLocalPosition(270.f, 130.f);
			prompt->AddComponent<TextComponent>(fontSmall, "Enter your name:");
			m_pScene->Add(std::move(prompt));

			// Display the 3 characters with the current one highlighted
			// Build string like "A B C" with brackets around active: "A [B] C"
			std::string nameDisplay;
			for (int i = 0; i < NameLength; ++i)
			{
				if (i > 0) nameDisplay += "  ";
				if (i == m_currentCharIndex)
					nameDisplay += std::string("[") + m_nameChars[i] + "]";
				else
					nameDisplay += std::string(" ") + m_nameChars[i] + " ";
			}

			auto nameGo = std::make_unique<GameObject>();
			nameGo->AddComponent<TransformComponent>()->SetLocalPosition(340.f, 170.f);
			nameGo->AddComponent<TextComponent>(fontLarge, nameDisplay);
			m_pScene->Add(std::move(nameGo));

			auto hint = std::make_unique<GameObject>();
			hint->AddComponent<TransformComponent>()->SetLocalPosition(200.f, 220.f);
			hint->AddComponent<TextComponent>(fontSmall,
				"W/S = change letter   Enter/A = confirm");
			m_pScene->Add(std::move(hint));
		}
		else
		{
			auto saved = std::make_unique<GameObject>();
			saved->AddComponent<TransformComponent>()->SetLocalPosition(280.f, 90.f);
			saved->AddComponent<TextComponent>(fontSmall, "Score saved! Press Enter to continue.");
			m_pScene->Add(std::move(saved));
		}

		// High score list
		const auto& highScores = GameSession::GetInstance().GetHighScores();
		float listStartY = m_entryComplete ? 140.f : 270.f;

		for (size_t i = 0; i < highScores.size(); ++i)
		{
			const auto& entry = highScores[i];
			std::ostringstream oss;
			oss << (i + 1) << ".  " << entry.name << "    " << entry.score;

			auto row = std::make_unique<GameObject>();
			row->AddComponent<TransformComponent>()->SetLocalPosition(
				250.f, listStartY + i * 28.f);
			row->AddComponent<TextComponent>(fontSmall, oss.str());
			m_pScene->Add(std::move(row));
		}

		if (highScores.empty())
		{
			auto noScores = std::make_unique<GameObject>();
			noScores->AddComponent<TransformComponent>()->SetLocalPosition(
				280.f, listStartY);
			noScores->AddComponent<TextComponent>(fontSmall, "No high scores yet!");
			m_pScene->Add(std::move(noScores));
		}
	}
}