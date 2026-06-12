#pragma once
#include "GameState.h"
#include "GameMode.h"
#include "LevelLoader.h"
#include <vector>
#include <string>

namespace dae
{
	class Scene;
	class GameObject;
	class TextComponent;
	class PlayerCollisionComponent;
	enum class VersusWinner;

	class PlayingState final : public GameState
	{
	public:
		PlayingState() = default;

		void OnEnter() override;
		void OnExit() override;
		void Update(float deltaTime) override;
		void FixedUpdate(float fixedTimeStep) override;
		void Render() const override;

		void SetGameMode(GameMode mode) { m_gameMode = mode; }
		GameMode GetGameMode() const { return m_gameMode; }

		int GetCurrentRound() const { return m_currentRound; }
		void SkipLevel();

	private:
		void LoadLevel(int round, int versusPlayerLives = VersusStartLives);
		void BindInput();
		void SoftReset();
		void RespawnSurvivingEnemies();   
		void AdvanceLevel();
		void WirePlayerCallbacks();
		void HandleGameOver();
		void SaveScoresToSession();
		std::string GetLevelFilePath(int round) const;

		// Co-op (shared lives)
		void OnCoopPlayerHit();           
		bool CoopHandleDeaths();          
		void CoopResetRound(PlayerCollisionComponent* c1, PlayerCollisionComponent* c2);
		void SetupCoopHUD();
		void UpdateCoopLivesHUD();

		// Versus mode
		void UpdateVersus();          
		void ResetVersusRound();      
		void SetupVersusHUD();        
		void UpdateFygarLivesHUD();
		void EndVersus(VersusWinner winner);

		GameMode m_gameMode{ GameMode::SinglePlayer };
		int m_currentRound{ 1 };
		int m_totalRounds{ 3 };

		// Versus state
		static constexpr int VersusStartLives{ 3 };
		int m_fygarLives{ VersusStartLives };
		TextComponent* m_pFygarLivesText{ nullptr };
		bool m_versusEnded{ false };
		bool m_pendingVersusReset{ false };

		// Co-op shared-lives state
		static constexpr int CoopStartLives{ 4 };
		int m_coopLives{ CoopStartLives };
		TextComponent* m_pCoopLivesText{ nullptr };
		bool m_coopEnded{ false };

		Scene* m_pScene{ nullptr };
		LevelData m_currentLevelData{};
		LevelBuildResult m_buildResult{};

		std::vector<std::string> m_levelFiles{
			"Data/Levels/level1.json",
			"Data/Levels/level2.json",
			"Data/Levels/level3.json"
		};
	};
}