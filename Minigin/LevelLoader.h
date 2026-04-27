#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "GameMode.h"

namespace dae
{
	class Scene;
	class GameObject;
	class GridComponent;

	struct EnemySpawn
	{
		enum class Type { Pooka, Fygar };
		Type type;
		int gridX;
		int gridY;
	};

	struct RockPosition
	{
		int gridX;
		int gridY;
	};

	struct LevelData
	{
		int gridWidth{ 14 };
		int gridHeight{ 14 };
		int surfaceRows{ 2 };
		std::vector<std::vector<int>> grid;
		glm::ivec2 playerSpawn{ 6, 2 };
		glm::ivec2 player2Spawn{ 8, 2 };
		std::vector<EnemySpawn> enemies;
		std::vector<RockPosition> rocks;
		int cellSize{ 32 };
		glm::ivec2 gridOffset{ 64, 48 };
	};

	// Returned by BuildScene so PlayingState can bind input to the right objects
	struct LevelBuildResult
	{
		GridComponent* pGrid{ nullptr };
		GameObject* pPlayer1{ nullptr };
		GameObject* pPlayer2{ nullptr };
		GameObject* pVersusEnemy{ nullptr };
		std::vector<GameObject*> enemies;
	};

	class LevelLoader final
	{
	public:
		static LevelData LoadFromFile(const std::string& filepath);
		static LevelBuildResult BuildScene(Scene& scene, const LevelData& data, GameMode mode);

	private:
		static GridComponent* CreateGrid(Scene& scene, const LevelData& data);
		static GameObject* CreatePlayer(Scene& scene, GridComponent* pGrid,
			const LevelData& data, int playerIndex);
		static std::vector<GameObject*> CreateEnemies(Scene& scene, GridComponent* pGrid,
			const LevelData& data, GameMode mode, GameObject*& outVersusEnemy);
		static void CreateRocks(Scene& scene, GridComponent* pGrid, const LevelData& data);
		static void CreateHUD(Scene& scene, GameMode mode);
	};
}