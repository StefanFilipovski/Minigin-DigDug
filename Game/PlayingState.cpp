#include "PlayingState.h"
#include "GameOverState.h"
#include "GameStateManager.h"
#include "GameSession.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "ServiceLocator.h"
#include "NullSoundService.h"
#include "SDLSoundService.h"
#include "GridMoveCommand.h"
#include "PumpCommand.h"
#include "Scene.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "TextComponent.h"
#include "PlayerCollisionComponent.h"
#include "ScoreComponent.h"
#include "RockComponent.h"
#include "EnemyComponent.h"
#include "Renderer.h"
#include "Command.h"
#include <SDL3/SDL.h>
#include <iostream>

namespace dae
{
	// ---- Playing-specific commands ----
	class SkipLevelCommand final : public Command
	{
	public:
		explicit SkipLevelCommand(PlayingState* pState) : m_pState(pState) {}
		void Execute() override { m_pState->SkipLevel(); }
	private:
		PlayingState* m_pState;
	};

	class MuteCommand final : public Command
	{
	public:
		void Execute() override
		{
			m_muted = !m_muted;
			if (m_muted)
			{
				ServiceLocator::RegisterSoundService(std::make_unique<NullSoundService>());
				std::cout << "[Sound Muted]\n";
			}
			else
			{
				ServiceLocator::RegisterSoundService(std::make_unique<SDLSoundService>());
				std::cout << "[Sound Unmuted]\n";
			}
		}
	private:
		static inline bool m_muted{ false };
	};

	// F3 = play a test sound to verify the sound system works
	class TestSoundCommand final : public Command
	{
	public:
		void Execute() override
		{
			ServiceLocator::GetSoundService().PlaySound("Data/pop.wav");
			std::cout << "[Test Sound Played]\n";
		}
	};

	// ---- PlayingState implementation ----
	void PlayingState::OnEnter()
	{
		auto& session = GameSession::GetInstance();
		m_gameMode = session.GetGameMode();
		m_currentRound = session.GetCurrentRound();

		auto& sceneMgr = SceneManager::GetInstance();

		m_pScene = sceneMgr.GetScene("Game");
		if (!m_pScene)
		{
			auto& scene = sceneMgr.CreateScene("Game");
			m_pScene = &scene;
		}

		sceneMgr.SetActiveScene("Game");
		LoadLevel(m_currentRound);
		BindInput();

		// Register overlay callback for black screen transitions
		Renderer::GetInstance().SetPostRenderCallback([this]()
		{
			if (!m_buildResult.pPlayer1) return;
			auto* collision = m_buildResult.pPlayer1->GetComponent<PlayerCollisionComponent>();
			if (collision && collision->IsInBlackScreen())
			{
				auto* renderer = Renderer::GetInstance().GetSDLRenderer();
				SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
				SDL_FRect fullScreen{ 0.f, 0.f, 640.f, 480.f };
				SDL_RenderFillRect(renderer, &fullScreen);
			}
		});
	}

	void PlayingState::OnExit()
	{
		Renderer::GetInstance().ClearPostRenderCallback();

		auto& session = GameSession::GetInstance();
		session.SetCurrentRound(m_currentRound);
	}

	void PlayingState::Update(float deltaTime)
	{
		(void)deltaTime;
	}

	void PlayingState::FixedUpdate(float fixedTimeStep)
	{
		(void)fixedTimeStep;
	}

	void PlayingState::Render() const
	{
	}

	void PlayingState::LoadLevel(int round)
	{
		if (!m_pScene) return;

		try
		{
			std::string filepath = GetLevelFilePath(round);
			m_currentLevelData = LevelLoader::LoadFromFile(filepath);
			m_buildResult = LevelLoader::BuildScene(*m_pScene, m_currentLevelData, m_gameMode);

			// Wire death callbacks on player collision component
			if (m_buildResult.pPlayer1)
			{
				auto* collision = m_buildResult.pPlayer1->GetComponent<PlayerCollisionComponent>();
				if (collision)
				{
					collision->SetSoftResetCallback([this]() { SoftReset(); });
					collision->SetGameOverCallback([this]()
					{
						// Save score to session before transitioning
						auto& session = GameSession::GetInstance();
						if (m_buildResult.pScore1)
							session.SetPlayer1Score(m_buildResult.pScore1->GetScore());
						session.SetCurrentRound(m_currentRound);

						Renderer::GetInstance().ClearPostRenderCallback();
						GameStateManager::GetInstance().SetState<GameOverState>();
					});
				}
			}

			std::cout << "[PlayingState] Loaded round " << round << " from " << filepath << "\n";
		}
		catch (const std::exception& e)
		{
			std::cerr << "[PlayingState] Failed to load level: " << e.what() << "\n";
			m_currentLevelData = LevelData{};
			m_buildResult = LevelLoader::BuildScene(*m_pScene, m_currentLevelData, m_gameMode);
		}
	}

	void PlayingState::BindInput()
	{
		auto& input = InputManager::GetInstance();
		input.ClearAllBindings();

		// F1 = skip level
		input.BindKeyboardCommand(SDL_SCANCODE_F1, KeyState::Down,
			std::make_unique<SkipLevelCommand>(this));

		// F2 = mute/unmute
		input.BindKeyboardCommand(SDL_SCANCODE_F2, KeyState::Down,
			std::make_unique<MuteCommand>());

		// F3 = test sound
		input.BindKeyboardCommand(SDL_SCANCODE_F3, KeyState::Down,
			std::make_unique<TestSoundCommand>());

		// Player 1: WASD (keyboard)
		if (m_buildResult.pPlayer1)
		{
			auto* p1 = m_buildResult.pPlayer1;

			input.BindKeyboardCommand(SDL_SCANCODE_W, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p1, glm::ivec2{ 0, -1 }));
			input.BindKeyboardCommand(SDL_SCANCODE_S, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p1, glm::ivec2{ 0, 1 }));
			input.BindKeyboardCommand(SDL_SCANCODE_A, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p1, glm::ivec2{ -1, 0 }));
			input.BindKeyboardCommand(SDL_SCANCODE_D, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p1, glm::ivec2{ 1, 0 }));

			// Pump attack: Space (keyboard) and A (controller)
			// Pressed = fires every frame while held, so PumpComponent can
			// distinguish tap vs hold and auto-pump at a slower rate.
			input.BindKeyboardCommand(SDL_SCANCODE_SPACE, KeyState::Pressed,
				std::make_unique<PumpCommand>(p1));

			// Player 1: Controller 0 (DPad)
			input.BindControllerCommand(0, Controller::Button::DPadUp, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p1, glm::ivec2{ 0, -1 }));
			input.BindControllerCommand(0, Controller::Button::DPadDown, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p1, glm::ivec2{ 0, 1 }));
			input.BindControllerCommand(0, Controller::Button::DPadLeft, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p1, glm::ivec2{ -1, 0 }));
			input.BindControllerCommand(0, Controller::Button::DPadRight, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p1, glm::ivec2{ 1, 0 }));

			// Pump on controller
			input.BindControllerCommand(0, Controller::Button::A, KeyState::Pressed,
				std::make_unique<PumpCommand>(p1));
		}

		// Player 2: Controller 1 (co-op as second Dig Dug)
		if (m_buildResult.pPlayer2)
		{
			auto* p2 = m_buildResult.pPlayer2;

			input.BindControllerCommand(1, Controller::Button::DPadUp, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ 0, -1 }));
			input.BindControllerCommand(1, Controller::Button::DPadDown, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ 0, 1 }));
			input.BindControllerCommand(1, Controller::Button::DPadLeft, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ -1, 0 }));
			input.BindControllerCommand(1, Controller::Button::DPadRight, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ 1, 0 }));
		}

		// Versus: Player 2 controls the Fygar with Controller 1
		if (m_buildResult.pVersusEnemy)
		{
			auto* fygar = m_buildResult.pVersusEnemy;

			input.BindControllerCommand(1, Controller::Button::DPadUp, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(fygar, glm::ivec2{ 0, -1 }));
			input.BindControllerCommand(1, Controller::Button::DPadDown, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(fygar, glm::ivec2{ 0, 1 }));
			input.BindControllerCommand(1, Controller::Button::DPadLeft, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(fygar, glm::ivec2{ -1, 0 }));
			input.BindControllerCommand(1, Controller::Button::DPadRight, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(fygar, glm::ivec2{ 1, 0 }));

			// TODO: A button = fire breath for versus Fygar
		}
	}

	void PlayingState::SoftReset()
	{
		if (!m_pScene) return;

		// Mark all old enemies for destruction
		for (auto* enemy : m_buildResult.enemies)
		{
			if (enemy && !enemy->IsMarkedForDestroy())
				enemy->MarkForDestroy();
		}

		// Re-create enemies from the original level data
		auto newEnemies = LevelLoader::RespawnEnemies(
			*m_pScene, m_buildResult.pGrid, m_currentLevelData, m_gameMode,
			m_buildResult.pPlayer1, m_buildResult.pPlayer2,
			m_buildResult.pScore1, m_buildResult.pScore2);

		// Rewire surviving rocks with new enemy references
		for (auto* rock : m_buildResult.rocks)
		{
			if (!rock || rock->IsMarkedForDestroy()) continue;
			auto* rockComp = rock->GetComponent<RockComponent>();
			if (!rockComp) continue;

			rockComp->ClearEnemies();
			for (auto* enemy : newEnemies)
				rockComp->AddEnemy(enemy);
		}

		m_buildResult.enemies = newEnemies;

		// Rebind input since enemies changed
		BindInput();

		// Re-set callbacks (they persist, but just to be safe after rebind)
		if (m_buildResult.pPlayer1)
		{
			auto* collision = m_buildResult.pPlayer1->GetComponent<PlayerCollisionComponent>();
			if (collision)
			{
				collision->SetSoftResetCallback([this]() { SoftReset(); });
				collision->SetGameOverCallback([this]()
				{
					auto& session = GameSession::GetInstance();
					if (m_buildResult.pScore1)
						session.SetPlayer1Score(m_buildResult.pScore1->GetScore());
					session.SetCurrentRound(m_currentRound);

					Renderer::GetInstance().ClearPostRenderCallback();
					GameStateManager::GetInstance().SetState<GameOverState>();
				});
			}
		}

		std::cout << "[PlayingState] Soft reset — enemies respawned, terrain preserved\n";
	}

	void PlayingState::SkipLevel()
	{
		++m_currentRound;
		if (m_currentRound > m_totalRounds)
		{
			GameStateManager::GetInstance().SetState<GameOverState>();
			return;
		}
		LoadLevel(m_currentRound);
		BindInput();
	}

	std::string PlayingState::GetLevelFilePath(int round) const
	{
		if (round > 0 && round <= static_cast<int>(m_levelFiles.size()))
			return m_levelFiles[round - 1];

		int index = (round - 1) % static_cast<int>(m_levelFiles.size());
		return m_levelFiles[index];
	}
}