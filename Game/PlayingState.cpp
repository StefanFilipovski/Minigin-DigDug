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
#include <algorithm>
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
		// In co-op, either player dying triggers the black screen
		Renderer::GetInstance().SetPostRenderCallback([this]()
		{
			auto isBlack = [](GameObject* pPlayer) -> bool
			{
				if (!pPlayer) return false;
				auto* c = pPlayer->GetComponent<PlayerCollisionComponent>();
				return c && c->IsInBlackScreen();
			};

			if (isBlack(m_buildResult.pPlayer1) || isBlack(m_buildResult.pPlayer2))
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

		// Nullify destroyed enemy pointers (instead of erasing) to preserve
		// the 1:1 index correspondence with m_currentLevelData.enemies.
		// This is critical for SoftReset, which correlates by index to decide
		// which enemies to respawn.
		for (auto& enemy : m_buildResult.enemies)
		{
			if (enemy && enemy->IsMarkedForDestroy())
				enemy = nullptr;
		}

		// Rocks don't have an index correlation, so erase-remove is fine.
		m_buildResult.rocks.erase(
			std::remove_if(m_buildResult.rocks.begin(), m_buildResult.rocks.end(),
				[](const GameObject* go) { return !go || go->IsMarkedForDestroy(); }),
			m_buildResult.rocks.end());

		// Don't check win condition while any player is in the death sequence
		auto isPlayerDying = [](GameObject* p) -> bool
		{
			if (!p) return false;
			auto* c = p->GetComponent<PlayerCollisionComponent>();
			return c && c->IsDead();
		};
		if (isPlayerDying(m_buildResult.pPlayer1) || isPlayerDying(m_buildResult.pPlayer2))
			return;

		// Check if all enemies are dead — level complete
		if (!m_buildResult.enemies.empty())
		{
			bool allDead = true;
			for (auto* enemy : m_buildResult.enemies)
			{
				if (enemy && !enemy->IsMarkedForDestroy())
				{
					auto* comp = enemy->GetComponent<EnemyComponent>();
					if (comp && comp->IsAlive())
					{
						allDead = false;
						break;
					}
				}
			}

			if (allDead)
			{
				AdvanceLevel();
			}
		}
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

			// Wire death callbacks on both players' collision components
			WirePlayerCallbacks();

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

		// Player 2: Arrow keys (keyboard) + Controller 1 (co-op as second Dig Dug)
		if (m_buildResult.pPlayer2)
		{
			auto* p2 = m_buildResult.pPlayer2;

			// Keyboard: Arrow keys + Right Shift for pump
			input.BindKeyboardCommand(SDL_SCANCODE_UP, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ 0, -1 }));
			input.BindKeyboardCommand(SDL_SCANCODE_DOWN, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ 0, 1 }));
			input.BindKeyboardCommand(SDL_SCANCODE_LEFT, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ -1, 0 }));
			input.BindKeyboardCommand(SDL_SCANCODE_RIGHT, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ 1, 0 }));
			input.BindKeyboardCommand(SDL_SCANCODE_RSHIFT, KeyState::Pressed,
				std::make_unique<PumpCommand>(p2));

			// Controller 1
			input.BindControllerCommand(1, Controller::Button::DPadUp, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ 0, -1 }));
			input.BindControllerCommand(1, Controller::Button::DPadDown, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ 0, 1 }));
			input.BindControllerCommand(1, Controller::Button::DPadLeft, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ -1, 0 }));
			input.BindControllerCommand(1, Controller::Button::DPadRight, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ 1, 0 }));
			input.BindControllerCommand(1, Controller::Button::A, KeyState::Pressed,
				std::make_unique<PumpCommand>(p2));
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

		// Build a filtered level data that only includes enemies that were
		// alive when the player died — dead enemies stay dead
		LevelData resetData = m_currentLevelData;
		resetData.enemies.clear();

		for (size_t i = 0; i < m_buildResult.enemies.size() && i < m_currentLevelData.enemies.size(); ++i)
		{
			auto* enemy = m_buildResult.enemies[i];
			bool wasAlive = false;

			if (enemy && !enemy->IsMarkedForDestroy())
			{
				auto* comp = enemy->GetComponent<EnemyComponent>();
				if (comp && comp->IsAlive())
					wasAlive = true;
			}

			if (wasAlive)
				resetData.enemies.push_back(m_currentLevelData.enemies[i]);
		}

		// Mark all old enemies for destruction
		for (auto* enemy : m_buildResult.enemies)
		{
			if (enemy && !enemy->IsMarkedForDestroy())
				enemy->MarkForDestroy();
		}

		// Re-create only the surviving enemies at their original spawn positions
		auto newEnemies = LevelLoader::RespawnEnemies(
			*m_pScene, m_buildResult.pGrid, resetData, m_gameMode,
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

		// Sync level data so the next SoftReset has a correct 1:1 index
		// correspondence between m_buildResult.enemies and m_currentLevelData.enemies
		m_currentLevelData.enemies = resetData.enemies;

		// Rebind input since enemies changed
		BindInput();

		// Re-set callbacks (they persist, but just to be safe after rebind)
		WirePlayerCallbacks();

		std::cout << "[PlayingState] Soft reset — enemies respawned, terrain preserved\n";
	}

	void PlayingState::WirePlayerCallbacks()
	{
		auto wireOne = [this](GameObject* pPlayer)
		{
			if (!pPlayer) return;
			auto* collision = pPlayer->GetComponent<PlayerCollisionComponent>();
			if (!collision) return;

			collision->SetSoftResetCallback([this]() { SoftReset(); });
			collision->SetGameOverCallback([this]() { HandleGameOver(); });
		};

		wireOne(m_buildResult.pPlayer1);
		wireOne(m_buildResult.pPlayer2);
	}

	void PlayingState::HandleGameOver()
	{
		// In co-op, only transition to game over when ALL players are out of lives.
		// The player who just ran out calls this, but the other might still be alive.
		if (m_gameMode == GameMode::CoOp)
		{
			auto hasLives = [](GameObject* p) -> bool
			{
				if (!p) return false;
				auto* c = p->GetComponent<PlayerCollisionComponent>();
				return c && c->GetLives() > 0;
			};

			if (hasLives(m_buildResult.pPlayer1) || hasLives(m_buildResult.pPlayer2))
				return; // Other player still alive — don't end the game
		}

		SaveScoresToSession();
		Renderer::GetInstance().ClearPostRenderCallback();
		GameStateManager::GetInstance().SetState<GameOverState>();
	}

	void PlayingState::SaveScoresToSession()
	{
		auto& session = GameSession::GetInstance();
		if (m_buildResult.pScore1)
			session.SetPlayer1Score(m_buildResult.pScore1->GetScore());
		if (m_buildResult.pScore2)
			session.SetPlayer2Score(m_buildResult.pScore2->GetScore());
		session.SetCurrentRound(m_currentRound);
	}

	void PlayingState::AdvanceLevel()
	{
		SaveScoresToSession();

		++m_currentRound;
		if (m_currentRound > m_totalRounds)
		{
			// Beat all levels — go to game over (victory)
			Renderer::GetInstance().ClearPostRenderCallback();
			GameStateManager::GetInstance().SetState<GameOverState>();
			return;
		}

		LoadLevel(m_currentRound);
		BindInput();

		std::cout << "[PlayingState] Level complete! Advancing to round " << m_currentRound << "\n";
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