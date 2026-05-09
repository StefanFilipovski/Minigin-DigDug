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
		void LoadLevel(int round);
		void BindInput();
		std::string GetLevelFilePath(int round) const;

		GameMode m_gameMode{ GameMode::SinglePlayer };
		int m_currentRound{ 1 };
		int m_totalRounds{ 3 };

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