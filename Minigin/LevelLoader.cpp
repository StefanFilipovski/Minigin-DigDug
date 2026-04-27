#include "LevelLoader.h"
#include "GridComponent.h"
#include "GridMovementComponent.h"
#include "SpriteAnimatorComponent.h"
#include "PlayerAnimController.h"
#include "PumpComponent.h"
#include "Scene.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "RenderComponent.h"
#include "TextComponent.h"
#include "ResourceManager.h"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <json.hpp>

namespace dae
{
	LevelData LevelLoader::LoadFromFile(const std::string& filepath)
	{
		std::ifstream file(filepath);
		if (!file.is_open())
			throw std::runtime_error("LevelLoader: Could not open file: " + filepath);

		nlohmann::json j;
		file >> j;

		LevelData data;
		data.gridWidth = j.value("gridWidth", 14);
		data.gridHeight = j.value("gridHeight", 14);
		data.surfaceRows = j.value("surfaceRows", 2);
		data.cellSize = j.value("cellSize", 32);

		if (j.contains("gridOffset"))
		{
			data.gridOffset.x = j["gridOffset"].value("x", 64);
			data.gridOffset.y = j["gridOffset"].value("y", 48);
		}

		if (j.contains("grid"))
		{
			data.grid.clear();
			for (const auto& row : j["grid"])
			{
				std::vector<int> gridRow;
				for (const auto& cell : row)
					gridRow.push_back(cell.get<int>());
				data.grid.push_back(gridRow);
			}
		}
		else
		{
			data.grid.resize(data.gridHeight);
			for (int y = 0; y < data.gridHeight; ++y)
				data.grid[y].resize(data.gridWidth, y < data.surfaceRows ? 2 : 0);
		}

		if (j.contains("playerSpawn"))
		{
			data.playerSpawn.x = j["playerSpawn"].value("x", 6);
			data.playerSpawn.y = j["playerSpawn"].value("y", 2);
		}
		if (j.contains("player2Spawn"))
		{
			data.player2Spawn.x = j["player2Spawn"].value("x", 8);
			data.player2Spawn.y = j["player2Spawn"].value("y", 2);
		}

		if (j.contains("enemies"))
		{
			for (const auto& enemy : j["enemies"])
			{
				EnemySpawn spawn;
				std::string typeStr = enemy.value("type", "Pooka");
				spawn.type = (typeStr == "Fygar") ? EnemySpawn::Type::Fygar : EnemySpawn::Type::Pooka;
				spawn.gridX = enemy.value("x", 0);
				spawn.gridY = enemy.value("y", 0);
				data.enemies.push_back(spawn);
			}
		}

		if (j.contains("rocks"))
		{
			for (const auto& rock : j["rocks"])
			{
				RockPosition pos;
				pos.gridX = rock.value("x", 0);
				pos.gridY = rock.value("y", 0);
				data.rocks.push_back(pos);
			}
		}

		return data;
	}

	LevelBuildResult LevelLoader::BuildScene(Scene& scene, const LevelData& data, GameMode mode)
	{
		scene.RemoveAll();

		LevelBuildResult result;

		result.pGrid = CreateGrid(scene, data);
		result.pPlayer1 = CreatePlayer(scene, result.pGrid, data, 0);

		if (mode == GameMode::CoOp)
			result.pPlayer2 = CreatePlayer(scene, result.pGrid, data, 1);

		result.enemies = CreateEnemies(scene, result.pGrid, data, mode, result.pVersusEnemy);

		CreateRocks(scene, result.pGrid, data);
		CreateHUD(scene, mode);

		return result;
	}

	GridComponent* LevelLoader::CreateGrid(Scene& scene, const LevelData& data)
	{
		auto gridGo = std::make_unique<GameObject>();
		gridGo->AddComponent<TransformComponent>();

		auto* pGrid = gridGo->AddComponent<GridComponent>(
			data.gridWidth, data.gridHeight, data.cellSize, data.gridOffset);

		// Initialize grid cells from level data
		for (int y = 0; y < data.gridHeight; ++y)
		{
			for (int x = 0; x < data.gridWidth; ++x)
			{
				int cellValue = 0;
				if (y < static_cast<int>(data.grid.size()) &&
					x < static_cast<int>(data.grid[y].size()))
				{
					cellValue = data.grid[y][x];
				}

				switch (cellValue)
				{
				case 0: pGrid->SetCellType(x, y, CellType::Dirt); break;
				case 1: pGrid->SetCellType(x, y, CellType::Tunnel); break;
				case 2: pGrid->SetCellType(x, y, CellType::Surface); break;
				default: pGrid->SetCellType(x, y, CellType::Dirt); break;
				}
			}
		}

		// GridComponent::Render() draws the grid every frame, so no individual
		// tile GameObjects needed — the grid updates visually when cells are dug.

		// Dig out the player spawn positions
		pGrid->SetCellType(data.playerSpawn.x, data.playerSpawn.y, CellType::Tunnel);
		pGrid->SetCellType(data.player2Spawn.x, data.player2Spawn.y, CellType::Tunnel);

		GridComponent* ptr = pGrid;
		scene.Add(std::move(gridGo));
		return ptr;
	}

	GameObject* LevelLoader::CreatePlayer(Scene& scene, GridComponent* pGrid,
		const LevelData& data, int playerIndex)
	{
		auto player = std::make_unique<GameObject>();

		glm::ivec2 spawnGrid = (playerIndex == 0) ? data.playerSpawn : data.player2Spawn;
		glm::vec2 spawnPixel = pGrid->GridToPixel(spawnGrid.x, spawnGrid.y);

		player->AddComponent<TransformComponent>()->SetLocalPosition(spawnPixel.x, spawnPixel.y);

		// Sprite rendering — use the spritesheet with source rects
		auto* render = player->AddComponent<RenderComponent>();
		render->SetTexture("general_sprites.png");

		// Set up sprite animator with Dig Dug walking animations
		// Spritesheet layout: 16x16 grid
		// Row 0 (y=0): walk right1, right2, up1, up2, left1, left2, down1, down2
		// Row 1 (y=16): dig right1, right2, dig up1, up2, dig left1, left2, dig down1, down2
		constexpr int S = 16; // Sprite size on the sheet
		float cellSize = static_cast<float>(data.cellSize); // Render at grid cell size

		auto* animator = player->AddComponent<SpriteAnimatorComponent>();
		animator->SetRenderSize(cellSize, cellSize);

		animator->AddAnimation("walk_right",
			{ {0 * S, 0, S, S}, {1 * S, 0, S, S} }, 8.f);
		animator->AddAnimation("walk_up",
			{ {2 * S, 0, S, S}, {3 * S, 0, S, S} }, 8.f);
		animator->AddAnimation("walk_left",
			{ {4 * S, 0, S, S}, {5 * S, 0, S, S} }, 8.f);
		animator->AddAnimation("walk_down",
			{ {6 * S, 0, S, S}, {7 * S, 0, S, S} }, 8.f);

		animator->AddAnimation("dig_right",
			{ {0 * S, S, S, S}, {1 * S, S, S, S} }, 8.f);
		animator->AddAnimation("dig_up",
			{ {2 * S, S, S, S}, {3 * S, S, S, S} }, 8.f);
		animator->AddAnimation("dig_left",
			{ {4 * S, S, S, S}, {5 * S, S, S, S} }, 8.f);
		animator->AddAnimation("dig_down",
			{ {6 * S, S, S, S}, {7 * S, S, S, S} }, 8.f);

		// Row 1 cols 8-15: Pump pose (holding hose extended)
		// right(2), up(2), left(2), down(2)
		animator->AddAnimation("pump_right",
			{ {8 * S, S, S, S}, {9 * S, S, S, S} }, 6.f);
		animator->AddAnimation("pump_up",
			{ {10 * S, S, S, S}, {11 * S, S, S, S} }, 6.f);
		animator->AddAnimation("pump_left",
			{ {12 * S, S, S, S}, {13 * S, S, S, S} }, 6.f);
		animator->AddAnimation("pump_down",
			{ {14 * S, S, S, S}, {15 * S, S, S, S} }, 6.f);

		// Start facing right
		animator->Play("walk_right");
		animator->Pause();

		// Animation controller watches movement and picks the right anim
		player->AddComponent<PlayerAnimController>();

		// Grid movement: speed = 4 cells/second, canDig = true
		auto* movement = player->AddComponent<GridMovementComponent>(pGrid, 4.f, true);
		movement->SetGridPosition(spawnGrid.x, spawnGrid.y);

		// Pump attack
		player->AddComponent<PumpComponent>(pGrid);

		GameObject* ptr = player.get();
		scene.Add(std::move(player));
		return ptr;
	}

	std::vector<GameObject*> LevelLoader::CreateEnemies(Scene& scene, GridComponent* pGrid,
		const LevelData& data, GameMode mode, GameObject*& outVersusEnemy)
	{
		std::vector<GameObject*> enemies;
		outVersusEnemy = nullptr;

		for (size_t i = 0; i < data.enemies.size(); ++i)
		{
			const auto& spawn = data.enemies[i];
			auto enemy = std::make_unique<GameObject>();

			glm::vec2 spawnPixel = pGrid->GridToPixel(spawn.gridX, spawn.gridY);
			enemy->AddComponent<TransformComponent>()->SetLocalPosition(spawnPixel.x, spawnPixel.y);
			enemy->AddComponent<RenderComponent>();
			auto* enemyRender = enemy->GetComponent<RenderComponent>();
			enemyRender->SetTexture(
				spawn.type == EnemySpawn::Type::Pooka ? "pooka.png" : "fygar.png");

			// Enemies move through tunnels, speed slightly different per type
			float enemySpeed = (spawn.type == EnemySpawn::Type::Pooka) ? 3.5f : 3.0f;
			auto* movement = enemy->AddComponent<GridMovementComponent>(pGrid, enemySpeed, false);
			movement->SetGridPosition(spawn.gridX, spawn.gridY);

			// Make sure enemy spawn cell is a tunnel
			pGrid->SetCellType(spawn.gridX, spawn.gridY, CellType::Tunnel);

			// In Versus mode, first Fygar is player-controlled
			bool isVersusPlayer = (mode == GameMode::Versus &&
				spawn.type == EnemySpawn::Type::Fygar &&
				outVersusEnemy == nullptr);

			GameObject* ptr = enemy.get();
			enemies.push_back(ptr);

			if (isVersusPlayer)
				outVersusEnemy = ptr;

			scene.Add(std::move(enemy));
		}

		return enemies;
	}

	void LevelLoader::CreateRocks(Scene& scene, GridComponent* pGrid, const LevelData& data)
	{
		for (const auto& rockPos : data.rocks)
		{
			auto rock = std::make_unique<GameObject>();
			glm::vec2 pos = pGrid->GridToPixel(rockPos.gridX, rockPos.gridY);
			rock->AddComponent<TransformComponent>()->SetLocalPosition(pos.x, pos.y);
			rock->AddComponent<RenderComponent>();
			rock->GetComponent<RenderComponent>()->SetTexture("rock.png");

			scene.Add(std::move(rock));
		}
	}

	void LevelLoader::CreateHUD(Scene& scene, GameMode mode)
	{
		auto font = ResourceManager::GetInstance().LoadFont("Lingua.otf", 18);

		auto scoreGo = std::make_unique<GameObject>();
		scoreGo->AddComponent<TransformComponent>()->SetLocalPosition(10.f, 5.f);
		scoreGo->AddComponent<TextComponent>(font, "Score: 0");
		scene.Add(std::move(scoreGo));

		auto livesGo = std::make_unique<GameObject>();
		livesGo->AddComponent<TransformComponent>()->SetLocalPosition(10.f, 25.f);
		livesGo->AddComponent<TextComponent>(font, "Lives: 4");
		scene.Add(std::move(livesGo));

		auto roundGo = std::make_unique<GameObject>();
		roundGo->AddComponent<TransformComponent>()->SetLocalPosition(400.f, 5.f);
		roundGo->AddComponent<TextComponent>(font, "Round: 1");
		scene.Add(std::move(roundGo));

		if (mode == GameMode::CoOp)
		{
			auto score2Go = std::make_unique<GameObject>();
			score2Go->AddComponent<TransformComponent>()->SetLocalPosition(10.f, 550.f);
			score2Go->AddComponent<TextComponent>(font, "P2 Score: 0");
			scene.Add(std::move(score2Go));
		}

		if (mode == GameMode::Versus)
		{
			auto versusGo = std::make_unique<GameObject>();
			versusGo->AddComponent<TransformComponent>()->SetLocalPosition(300.f, 550.f);
			versusGo->AddComponent<TextComponent>(font, "VERSUS MODE");
			scene.Add(std::move(versusGo));
		}
	}
}