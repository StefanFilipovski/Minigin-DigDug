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
#include "FireBreathCommand.h"
#include "GhostCommand.h"
#include "GameSounds.h"
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
	// Playing-specific commands
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
			ServiceLocator::GetSoundService().PlaySound(Sounds::MonsterBlow);
			std::cout << "[Test Sound Played]\n";
		}
	};

	// PlayingState implementation
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

		// Fresh Versus match state
		if (m_gameMode == GameMode::Versus)
		{
			m_fygarLives = VersusStartLives;
			m_versusEnded = false;
			m_pendingVersusReset = false;
		}

		// Fresh co-op shared-lives pool (persists across levels)
		if (m_gameMode == GameMode::CoOp)
		{
			m_coopLives = CoopStartLives;
			m_coopEnded = false;
		}

		LoadLevel(m_currentRound);
		BindInput();

		// Full-screen black-out while a player is in the black-screen death phase
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

		// Looping gameplay music
		ServiceLocator::GetSoundService().PlayMusic(Sounds::GameMusic);
	}

	void PlayingState::OnExit()
	{
		Renderer::GetInstance().ClearPostRenderCallback();
		ServiceLocator::GetSoundService().StopMusic();

		auto& session = GameSession::GetInstance();
		session.SetCurrentRound(m_currentRound);
	}

	void PlayingState::Update(float deltaTime)
	{
		(void)deltaTime;

		// Versus: a death scheduled a position reset. Do it here (outside any
		// scene object's Update) so we never rebuild the Fygar mid-update.
		if (m_pendingVersusReset)
		{
			ResetVersusRound();
			return;
		}

		// Nullify (don't erase) destroyed enemies to keep their index aligned
		// with m_currentLevelData.enemies, which resets rely on.
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

		// Versus has its own win/lose + Fygar-respawn rules.
		if (m_gameMode == GameMode::Versus)
		{
			UpdateVersus();
			return;
		}

		// Co-op: handle shared-lives deaths; skips the win check while a death is in progress
		if (m_gameMode == GameMode::CoOp)
		{
			if (CoopHandleDeaths())
				return;
		}

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

	void PlayingState::LoadLevel(int round, int versusPlayerLives)
	{
		if (!m_pScene) return;

		try
		{
			std::string filepath = GetLevelFilePath(round);
			m_currentLevelData = LevelLoader::LoadFromFile(filepath);

			// Versus: the arena holds exactly the human Dig Dug and ONE
			// player-controlled Fygar — strip every other enemy from the data.
			if (m_gameMode == GameMode::Versus)
			{
				EnemySpawn fygarSpawn{};
				bool found = false;
				for (const auto& e : m_currentLevelData.enemies)
				{
					if (e.type == EnemySpawn::Type::Fygar)
					{
						fygarSpawn = e;
						found = true;
						break;
					}
				}
				m_currentLevelData.enemies.clear();
				if (found)
					m_currentLevelData.enemies.push_back(fygarSpawn);
			}

			m_buildResult = LevelLoader::BuildScene(*m_pScene, m_currentLevelData, m_gameMode,
				versusPlayerLives);

			// Wire death callbacks on both players' collision components
			WirePlayerCallbacks();

			// Versus: refresh the Fygar-lives HUD (player lives set in BuildScene)
			if (m_gameMode == GameMode::Versus)
			{
				m_versusEnded = false;
				SetupVersusHUD();
			}

			// Co-op: shared lives HUD (the count itself persists across levels)
			if (m_gameMode == GameMode::CoOp)
			{
				SetupCoopHUD();
				UpdateCoopLivesHUD();
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

		// Player 1: WASD + Space (keyboard always)
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
			input.BindKeyboardCommand(SDL_SCANCODE_SPACE, KeyState::Pressed,
				std::make_unique<PumpCommand>(p1));

			// In single player, P1 also gets Controller 0.
			// In co-op/versus, Controller 0 is reserved for P2.
			if (m_gameMode == GameMode::SinglePlayer)
			{
				input.BindControllerCommand(0, Controller::Button::DPadUp, KeyState::Pressed,
					std::make_unique<GridMoveCommand>(p1, glm::ivec2{ 0, -1 }));
				input.BindControllerCommand(0, Controller::Button::DPadDown, KeyState::Pressed,
					std::make_unique<GridMoveCommand>(p1, glm::ivec2{ 0, 1 }));
				input.BindControllerCommand(0, Controller::Button::DPadLeft, KeyState::Pressed,
					std::make_unique<GridMoveCommand>(p1, glm::ivec2{ -1, 0 }));
				input.BindControllerCommand(0, Controller::Button::DPadRight, KeyState::Pressed,
					std::make_unique<GridMoveCommand>(p1, glm::ivec2{ 1, 0 }));
				input.BindControllerCommand(0, Controller::Button::A, KeyState::Pressed,
					std::make_unique<PumpCommand>(p1));
			}
		}

		// Player 2 (co-op): Controller 0 + Arrow keys + Right Shift
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

			// Controller 0 (the first/only controller goes to P2 in co-op)
			input.BindControllerCommand(0, Controller::Button::DPadUp, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ 0, -1 }));
			input.BindControllerCommand(0, Controller::Button::DPadDown, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ 0, 1 }));
			input.BindControllerCommand(0, Controller::Button::DPadLeft, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ -1, 0 }));
			input.BindControllerCommand(0, Controller::Button::DPadRight, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(p2, glm::ivec2{ 1, 0 }));
			input.BindControllerCommand(0, Controller::Button::A, KeyState::Pressed,
				std::make_unique<PumpCommand>(p2));
		}

		// Versus: Player 2 controls the Fygar with Controller 0 only
		if (m_buildResult.pVersusEnemy)
		{
			auto* fygar = m_buildResult.pVersusEnemy;

			// Controller 0
			input.BindControllerCommand(0, Controller::Button::DPadUp, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(fygar, glm::ivec2{ 0, -1 }));
			input.BindControllerCommand(0, Controller::Button::DPadDown, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(fygar, glm::ivec2{ 0, 1 }));
			input.BindControllerCommand(0, Controller::Button::DPadLeft, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(fygar, glm::ivec2{ -1, 0 }));
			input.BindControllerCommand(0, Controller::Button::DPadRight, KeyState::Pressed,
				std::make_unique<GridMoveCommand>(fygar, glm::ivec2{ 1, 0 }));
			input.BindControllerCommand(0, Controller::Button::A, KeyState::Down,
				std::make_unique<FireBreathCommand>(fygar));
			// B = enter ghost form (phase through dirt, on a cooldown)
			input.BindControllerCommand(0, Controller::Button::B, KeyState::Down,
				std::make_unique<GhostCommand>(fygar));
		}
	}

	void PlayingState::SoftReset()
	{
		if (!m_pScene) return;

		// Versus: any death restarts the round; defer to the next Update so we
		// don't rebuild the Fygar from inside the dying player's own Update.
		if (m_gameMode == GameMode::Versus)
		{
			m_pendingVersusReset = true;
			std::cout << "[PlayingState] Versus — Dig Dug died, scheduling round reset\n";
			return;
		}

		// Single-player: respawn surviving enemies, terrain preserved.
		RespawnSurvivingEnemies();
		BindInput();
		WirePlayerCallbacks();

		std::cout << "[PlayingState] Soft reset — enemies respawned, terrain preserved\n";
	}

	// Re-creates the still-alive enemies at their spawns and rewires rocks
	// (terrain preserved). Used by the single-player and co-op resets.
	void PlayingState::RespawnSurvivingEnemies()
	{
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

		// Sync level data so the next reset keeps a correct 1:1 index
		// correspondence between m_buildResult.enemies and m_currentLevelData.enemies
		m_currentLevelData.enemies = resetData.enemies;
	}

	void PlayingState::WirePlayerCallbacks()
	{
		const bool coop = (m_gameMode == GameMode::CoOp);

		auto wireOne = [this, coop](GameObject* pPlayer)
		{
			if (!pPlayer) return;
			auto* collision = pPlayer->GetComponent<PlayerCollisionComponent>();
			if (!collision) return;

			collision->SetSoftResetCallback([this]() { SoftReset(); });
			collision->SetGameOverCallback([this]() { HandleGameOver(); });

			// In co-op, lives are shared and PlayingState coordinates death/respawn.
			collision->SetCoopShared(coop);
			if (coop)
				collision->SetOnDeathStartCallback([this]() { OnCoopPlayerHit(); });
		};

		wireOne(m_buildResult.pPlayer1);
		wireOne(m_buildResult.pPlayer2);
	}

	void PlayingState::HandleGameOver()
	{
		// Versus: Dig Dug ran out of lives — the Fygar wins.
		if (m_gameMode == GameMode::Versus)
		{
			EndVersus(VersusWinner::Fygar);
			return;
		}

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

	//  Versus mode

	void PlayingState::UpdateVersus()
	{
		if (m_versusEnded || m_pendingVersusReset) return;

		// Wait out Dig Dug's death sequence before resolving anything.
		if (m_buildResult.pPlayer1)
		{
			auto* pc = m_buildResult.pPlayer1->GetComponent<PlayerCollisionComponent>();
			if (pc && pc->IsDead())
				return;
		}

		// Has the Fygar been killed (popped or crushed)?
		bool fygarDead = false;
		if (!m_buildResult.pVersusEnemy || m_buildResult.pVersusEnemy->IsMarkedForDestroy())
		{
			fygarDead = true;
		}
		else
		{
			auto* ec = m_buildResult.pVersusEnemy->GetComponent<EnemyComponent>();
			if (!ec || !ec->IsAlive())
				fygarDead = true;
		}

		if (fygarDead)
		{
			--m_fygarLives;
			UpdateFygarLivesHUD();

			if (m_fygarLives <= 0)
				EndVersus(VersusWinner::Player); // Dig Dug wins
			else
				m_pendingVersusReset = true;     // restart the round next Update
		}
	}

	void PlayingState::ResetVersusRound()
	{
		m_pendingVersusReset = false;
		if (!m_pScene) return;

		// Send Dig Dug back to its spawn. The player object persists, so its
		// remaining lives (already decremented on death) are untouched.
		if (m_buildResult.pPlayer1)
		{
			if (auto* c = m_buildResult.pPlayer1->GetComponent<PlayerCollisionComponent>())
				c->Respawn(m_currentLevelData.playerSpawn.x, m_currentLevelData.playerSpawn.y);
		}

		// Re-create the Fygar at its spawn (grid is kept, so dug tunnels remain)
		if (m_buildResult.pVersusEnemy && !m_buildResult.pVersusEnemy->IsMarkedForDestroy())
			m_buildResult.pVersusEnemy->MarkForDestroy();

		auto newEnemies = LevelLoader::RespawnEnemies(
			*m_pScene, m_buildResult.pGrid, m_currentLevelData, m_gameMode,
			m_buildResult.pPlayer1, m_buildResult.pPlayer2,
			m_buildResult.pScore1, m_buildResult.pScore2);

		m_buildResult.enemies = newEnemies;
		m_buildResult.pVersusEnemy = newEnemies.empty() ? nullptr : newEnemies.front();

		// Rewire surviving rocks to the new Fygar.
		for (auto* rock : m_buildResult.rocks)
		{
			if (!rock || rock->IsMarkedForDestroy()) continue;
			auto* rockComp = rock->GetComponent<RockComponent>();
			if (!rockComp) continue;

			rockComp->ClearEnemies();
			for (auto* enemy : newEnemies)
				rockComp->AddEnemy(enemy);
		}

		UpdateFygarLivesHUD();   // count unchanged — keep the readout in sync
		BindInput();             // bind the controller to the new Fygar object

		// Versus never advances levels, so the music is only ever started in
		// OnEnter. Re-assert it on each round reset so it can't be left stopped.
		ServiceLocator::GetSoundService().PlayMusic(Sounds::GameMusic);

		std::cout << "[PlayingState] Versus round reset — positions reset, tunnels preserved\n";
	}

	void PlayingState::SetupVersusHUD()
	{
		if (!m_pScene) return;

		auto font = ResourceManager::GetInstance().LoadFont("Lingua.otf", 18);

		auto go = std::make_unique<GameObject>();
		go->AddComponent<TransformComponent>()->SetLocalPosition(480.f, 25.f);
		m_pFygarLivesText = go->AddComponent<TextComponent>(
			font, "Fygar Lives: " + std::to_string(m_fygarLives));
		m_pScene->Add(std::move(go));
	}

	void PlayingState::UpdateFygarLivesHUD()
	{
		if (m_pFygarLivesText)
		{
			int shown = (m_fygarLives < 0) ? 0 : m_fygarLives;
			m_pFygarLivesText->SetText("Fygar Lives: " + std::to_string(shown));
		}
	}

	void PlayingState::EndVersus(VersusWinner winner)
	{
		if (m_versusEnded) return;
		m_versusEnded = true;

		GameSession::GetInstance().SetVersusWinner(winner);
		SaveScoresToSession();
		Renderer::GetInstance().ClearPostRenderCallback();
		GameStateManager::GetInstance().SetState<GameOverState>();
	}

	//  Co-op (shared lives)

	void PlayingState::OnCoopPlayerHit()
	{
		// Called the instant a player is hit (from PlayerCollisionComponent).
		// Each death consumes one life from the shared pool.
		--m_coopLives;
		UpdateCoopLivesHUD();
	}

	bool PlayingState::CoopHandleDeaths()
	{
		if (m_coopEnded) return true;

		auto getCollision = [](GameObject* p) -> PlayerCollisionComponent*
		{
			return p ? p->GetComponent<PlayerCollisionComponent>() : nullptr;
		};
		auto* c1 = getCollision(m_buildResult.pPlayer1);
		auto* c2 = getCollision(m_buildResult.pPlayer2);

		const bool p1Dead = (c1 && c1->IsDead());
		const bool p2Dead = (c2 && c2->IsDead());

		// No death in progress — let the normal win check run.
		if (!p1Dead && !p2Dead)
			return false;

		// Wait until all dying players have finished their death animation before
		// resolving, so a simultaneous double-death resolves only once.
		const bool stillAnimating =
			(p1Dead && !c1->IsAwaitingRespawn()) ||
			(p2Dead && !c2->IsAwaitingRespawn());
		if (stillAnimating)
			return true;

		// All dying players have finished — resolve the shared-lives outcome.
		if (m_coopLives <= 0)
		{
			m_coopEnded = true;
			SaveScoresToSession();
			Renderer::GetInstance().ClearPostRenderCallback();
			GameStateManager::GetInstance().SetState<GameOverState>();
			return true;
		}

		CoopResetRound(c1, c2);
		return true;
	}

	void PlayingState::CoopResetRound(PlayerCollisionComponent* c1, PlayerCollisionComponent* c2)
	{
		// Respawn the surviving enemies to their spawns (terrain preserved).
		RespawnSurvivingEnemies();

		// Reposition BOTH players to their spawns — including a player who never
		// died — and clear the dead player's death state.
		if (m_buildResult.pPlayer1 && c1)
			c1->Respawn(m_currentLevelData.playerSpawn.x, m_currentLevelData.playerSpawn.y);
		if (m_buildResult.pPlayer2 && c2)
			c2->Respawn(m_currentLevelData.player2Spawn.x, m_currentLevelData.player2Spawn.y);

		// Edge case: if every enemy was cleared during the death sequence, the
		// round is actually complete — advance instead of sitting in an empty level.
		bool anyEnemyAlive = false;
		for (auto* enemy : m_buildResult.enemies)
		{
			if (enemy && !enemy->IsMarkedForDestroy())
			{
				auto* comp = enemy->GetComponent<EnemyComponent>();
				if (comp && comp->IsAlive()) { anyEnemyAlive = true; break; }
			}
		}
		if (!anyEnemyAlive)
			AdvanceLevel();

		std::cout << "[PlayingState] Co-op round reset — both players to spawn, "
			<< m_coopLives << " shared lives left\n";
	}

	void PlayingState::SetupCoopHUD()
	{
		if (!m_pScene) return;

		auto font = ResourceManager::GetInstance().LoadFont("Lingua.otf", 18);

		auto go = std::make_unique<GameObject>();
		go->AddComponent<TransformComponent>()->SetLocalPosition(10.f, 25.f);
		m_pCoopLivesText = go->AddComponent<TextComponent>(
			font, "Lives: " + std::to_string(m_coopLives));
		m_pScene->Add(std::move(go));
	}

	void PlayingState::UpdateCoopLivesHUD()
	{
		if (m_pCoopLivesText)
		{
			int shown = (m_coopLives < 0) ? 0 : m_coopLives;
			m_pCoopLivesText->SetText("Lives: " + std::to_string(shown));
		}
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

		// Re-assert the gameplay music for the new level so it can't be left
		// in a stopped state after the transition.
		ServiceLocator::GetSoundService().PlayMusic(Sounds::GameMusic);

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

		ServiceLocator::GetSoundService().PlayMusic(Sounds::GameMusic);
	}

	std::string PlayingState::GetLevelFilePath(int round) const
	{
		if (round > 0 && round <= static_cast<int>(m_levelFiles.size()))
			return m_levelFiles[round - 1];

		int index = (round - 1) % static_cast<int>(m_levelFiles.size());
		return m_levelFiles[index];
	}
}