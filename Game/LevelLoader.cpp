#include "LevelLoader.h"
#include "GridComponent.h"
#include "GridMovementComponent.h"
#include "SpriteAnimatorComponent.h"
#include "PlayerAnimController.h"
#include "PumpComponent.h"
#include "EnemyComponent.h"
#include "PlayerCollisionComponent.h"
#include "RockComponent.h"
#include "ScoreComponent.h"
#include "PointsDisplayComponent.h"
#include "LivesDisplayComponent.h"
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
#include <SDL3/SDL_pixels.h>

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

	LevelBuildResult LevelLoader::BuildScene(Scene& scene, const LevelData& data, GameMode mode,
		int versusPlayerLives)
	{
		scene.RemoveAll();

		LevelBuildResult result;

		result.pGrid = CreateGrid(scene, data);
		result.pPlayer1 = CreatePlayer(scene, result.pGrid, data, 0);

		if (mode == GameMode::CoOp)
			result.pPlayer2 = CreatePlayer(scene, result.pGrid, data, 1);

		// Versus: Dig Dug starts with the supplied life count (3 for a fresh
		// match, or the carried-over count on a round restart) — set this before
		// CreateHUD so the lives readout shows the correct value.
		if (mode == GameMode::Versus && result.pPlayer1)
		{
			if (auto* c = result.pPlayer1->GetComponent<PlayerCollisionComponent>())
				c->SetLives(versusPlayerLives);
		}

		result.enemies = CreateEnemies(scene, result.pGrid, data, mode,
			result.pVersusEnemy, result.pPlayer1);

		// Get score components before creating rocks (rocks need them for crush bonuses)
		ScoreComponent* pScore1 = result.pPlayer1 ?
			result.pPlayer1->GetComponent<ScoreComponent>() : nullptr;
		ScoreComponent* pScore2 = result.pPlayer2 ?
			result.pPlayer2->GetComponent<ScoreComponent>() : nullptr;

		// Set scene early so popups can spawn during gameplay
		if (pScore1) pScore1->SetScene(&scene);
		if (pScore2) pScore2->SetScene(&scene);

		result.rocks = CreateRocks(scene, result.pGrid, data,
			result.pPlayer1, result.pPlayer2, result.enemies,
			pScore1, pScore2);
		CreateHUD(scene, mode, pScore1, pScore2, result.pPlayer1, result.pPlayer2);

		// Wire up collision, pump, and scoring for each player
		auto wirePlayer = [&](GameObject* pPlayer, const glm::ivec2& spawnPos,
			ScoreComponent*& outScore)
		{
			if (!pPlayer) return;

			auto* collision = pPlayer->GetComponent<PlayerCollisionComponent>();
			if (collision)
			{
				collision->SetSpawnPos(spawnPos);
				for (auto* enemy : result.enemies)
					collision->AddEnemy(enemy);
			}

			auto* pump = pPlayer->GetComponent<PumpComponent>();
			if (pump)
			{
				for (auto* enemy : result.enemies)
					pump->AddEnemy(enemy);
			}

			outScore = pPlayer->GetComponent<ScoreComponent>();
			if (outScore)
			{
				for (auto* enemy : result.enemies)
				{
					auto* enemyComp = enemy->GetComponent<EnemyComponent>();
					if (enemyComp)
						enemyComp->AddObserver(outScore);
				}
			}
		};

		wirePlayer(result.pPlayer1, data.playerSpawn, result.pScore1);
		wirePlayer(result.pPlayer2, data.player2Spawn, result.pScore2);

		// In co-op, add Player 2 as an additional target so enemies chase
		// the closest player instead of always targeting Player 1
		if (result.pPlayer2)
		{
			for (auto* enemy : result.enemies)
			{
				auto* enemyComp = enemy->GetComponent<EnemyComponent>();
				if (enemyComp)
					enemyComp->AddTarget(result.pPlayer2);
			}
		}

		return result;
	}

	GridComponent* LevelLoader::CreateGrid(Scene& scene, const LevelData& data)
	{
		auto gridGo = std::make_unique<GameObject>();
		gridGo->AddComponent<TransformComponent>();

		auto* pGrid = gridGo->AddComponent<GridComponent>(
			data.gridWidth, data.gridHeight, data.cellSize, data.gridOffset);
		pGrid->SetSurfaceRows(data.surfaceRows);

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

		// Pre-cache the player sprite sheets with the red background keyed out.
		// Cached textures stay keyed for the rest of the session.
		SDL_Color redKey{ 108, 7, 0, 255 };
		ResourceManager::GetInstance().LoadTexture("DigDugMove1.png", redKey);
		ResourceManager::GetInstance().LoadTexture("DigDugMoveShovel.png", redKey);
		ResourceManager::GetInstance().LoadTexture("DigDugMoveShovelHole.png", redKey);
		ResourceManager::GetInstance().LoadTexture("DigDugDie.png", redKey);

		auto* render = player->AddComponent<RenderComponent>();
		render->SetTexture("DigDugMove1.png");

		// Each player sheet is 128×16: 8 frames of 16×16 (right×2, up×2, left×2, down×2)
		// DigDugMove1.png            — walk
		// DigDugMoveShovel.png       — dig
		// DigDugMoveShovelHole.png   — pump pose
		constexpr int S = 16;
		float cellSize = static_cast<float>(data.cellSize);

		auto* animator = player->AddComponent<SpriteAnimatorComponent>();
		animator->SetRenderSize(cellSize, cellSize);

		const std::string walkTex = "DigDugMove1.png";
		animator->AddAnimation("walk_right", walkTex,
			{ {0 * S, 0, S, S}, {1 * S, 0, S, S} }, 8.f);
		animator->AddAnimation("walk_up", walkTex,
			{ {2 * S, 0, S, S}, {3 * S, 0, S, S} }, 8.f);
		animator->AddAnimation("walk_left", walkTex,
			{ {4 * S, 0, S, S}, {5 * S, 0, S, S} }, 8.f);
		animator->AddAnimation("walk_down", walkTex,
			{ {6 * S, 0, S, S}, {7 * S, 0, S, S} }, 8.f);

		const std::string digTex = "DigDugMoveShovel.png";
		animator->AddAnimation("dig_right", digTex,
			{ {0 * S, 0, S, S}, {1 * S, 0, S, S} }, 8.f);
		animator->AddAnimation("dig_up", digTex,
			{ {2 * S, 0, S, S}, {3 * S, 0, S, S} }, 8.f);
		animator->AddAnimation("dig_left", digTex,
			{ {4 * S, 0, S, S}, {5 * S, 0, S, S} }, 8.f);
		animator->AddAnimation("dig_down", digTex,
			{ {6 * S, 0, S, S}, {7 * S, 0, S, S} }, 8.f);

		// Pump uses the same regular-shovel sheet as digging (no "hole" variant)
		const std::string pumpTex = "DigDugMoveShovel.png";
		animator->AddAnimation("pump_right", pumpTex,
			{ {0 * S, 0, S, S}, {1 * S, 0, S, S} }, 6.f);
		animator->AddAnimation("pump_up", pumpTex,
			{ {2 * S, 0, S, S}, {3 * S, 0, S, S} }, 6.f);
		animator->AddAnimation("pump_left", pumpTex,
			{ {4 * S, 0, S, S}, {5 * S, 0, S, S} }, 6.f);
		animator->AddAnimation("pump_down", pumpTex,
			{ {6 * S, 0, S, S}, {7 * S, 0, S, S} }, 6.f);

		// Death animation — DigDugDie.png: 62×16, 4 frames, non-looping
		const std::string dieTex = "DigDugDie.png";
		animator->AddAnimation("die", dieTex,
			{ {0, 0, S, S}, {S, 0, S, S}, {2 * S, 0, S, S}, {48, 0, 14, S} }, 4.f, false);

		animator->Play("walk_right");
		animator->Pause();

		player->AddComponent<PlayerAnimController>();

		// canDig = true
		auto* movement = player->AddComponent<GridMovementComponent>(pGrid, 4.f, true);
		movement->SetGridPosition(spawnGrid.x, spawnGrid.y);

		player->AddComponent<PumpComponent>(pGrid);
		player->AddComponent<ScoreComponent>();

		// Enemies are registered into this component after they're created
		player->AddComponent<PlayerCollisionComponent>(pGrid);

		GameObject* ptr = player.get();
		scene.Add(std::move(player));
		return ptr;
	}

	std::vector<GameObject*> LevelLoader::CreateEnemies(Scene& scene, GridComponent* pGrid,
		const LevelData& data, GameMode mode, GameObject*& outVersusEnemy,
		GameObject* pPlayerTarget)
	{
		std::vector<GameObject*> enemies;
		outVersusEnemy = nullptr;

		constexpr int S = 16; // Sprite size on general_sprites sheet
		float cellSize = static_cast<float>(data.cellSize);

		// Pre-cache enemy sheets with background keyed out
		SDL_Color redKey{ 108, 7, 0, 255 };
		ResourceManager::GetInstance().LoadTexture("Enemy1Walk.png", redKey);
		ResourceManager::GetInstance().LoadTexture("Enemy1Ghost.png", redKey);
		ResourceManager::GetInstance().LoadTexture("Enemy1Explode1.png", redKey);
		ResourceManager::GetInstance().LoadTexture("Enemy1Explode2.png", redKey);
		ResourceManager::GetInstance().LoadTexture("Enemy1Explode3.png", redKey);
		ResourceManager::GetInstance().LoadTexture("Enemy1Explode4.png", redKey);

		ResourceManager::GetInstance().LoadTexture("Enemy2Walk.png", redKey);
		ResourceManager::GetInstance().LoadTexture("Enemy2Ghost.png", redKey);
		ResourceManager::GetInstance().LoadTexture("Enemy2Fire.png", redKey);
		ResourceManager::GetInstance().LoadTexture("Enemy2Explode1.png", redKey);
		ResourceManager::GetInstance().LoadTexture("Enemy2Explode2.png", redKey);
		ResourceManager::GetInstance().LoadTexture("Enemy2Explode3.png", redKey);
		ResourceManager::GetInstance().LoadTexture("Enemy2Explode4.png", redKey);

		// Enemy1Walk.png layout: 2 rows × 3 columns, each frame 13×14 px
		//   Row 0 (y=0):  walk right — frames at x=0, 13, 26
		//   Row 1 (y=14): walk left  — frames at x=0, 13, 26
		constexpr int EW = 13;
		constexpr int EH = 14;

		for (size_t i = 0; i < data.enemies.size(); ++i)
		{
			const auto& spawn = data.enemies[i];
			auto enemy = std::make_unique<GameObject>();

			glm::vec2 spawnPixel = pGrid->GridToPixel(spawn.gridX, spawn.gridY);
			enemy->AddComponent<TransformComponent>()->SetLocalPosition(spawnPixel.x, spawnPixel.y);

			auto* render = enemy->AddComponent<RenderComponent>();

			auto* animator = enemy->AddComponent<SpriteAnimatorComponent>();
			animator->SetRenderSize(cellSize, cellSize);

			if (spawn.type == EnemySpawn::Type::Pooka)
			{
				render->SetTexture("Enemy1Walk.png");

				// Walk right/left from Enemy1Walk.png
				// Sheet: 46×30, 3 cols × 2 rows, 13×14 content per cell
				// Column stride = 15 (13 + 2px gap), row stride = 15 (14 + 1px gap)
				// Col starts: x=0, 15, 30  |  Row starts: y=0 (right), y=15 (left)
				const std::string walkTex = "Enemy1Walk.png";
				constexpr int ECS = 15; // cell stride
				animator->AddAnimation("walk_right", walkTex,
					{ {0, 0, EW, EH}, {ECS, 0, EW, EH}, {ECS * 2, 0, EW, EH} }, 6.f);
				animator->AddAnimation("walk_left", walkTex,
					{ {0, ECS, EW, EH}, {ECS, ECS, EW, EH}, {ECS * 2, ECS, EW, EH} }, 6.f);

				// No walk_up/walk_down — EnemyComponent calls Play("walk_up/down") which
				// silently no-ops, so the last horizontal animation keeps playing.

				// Enemy1Ghost.png: 1 row × 2 columns, each frame 14×8, sheet 28×8
				const std::string ghostTex = "Enemy1Ghost.png";
				constexpr int GW = 14;
				constexpr int GH = 8;
				animator->AddAnimation("ghost", ghostTex,
					{ {0, 0, GW, GH}, {GW, 0, GW, GH} }, 6.f);
				animator->SetAnimationRenderSize("ghost", cellSize * 0.6f, cellSize * 0.6f);

				// Inflate stages — one sprite per file, faces left by default.
				// EnemyComponent flips horizontally when attacked from the right.
				// Sizes: 14×14, 20×16, 21×20, 25×20
				animator->AddAnimation("inflate_1", std::string("Enemy1Explode1.png"),
					{ {0, 0, 14, 14} }, 1.f, false);
				animator->AddAnimation("inflate_2", std::string("Enemy1Explode2.png"),
					{ {0, 0, 20, 16} }, 1.f, false);
				animator->AddAnimation("inflate_3", std::string("Enemy1Explode3.png"),
					{ {0, 0, 21, 20} }, 1.f, false);
				animator->AddAnimation("inflate_4", std::string("Enemy1Explode4.png"),
					{ {0, 0, 25, 20} }, 1.f, false);
			}
			else // Fygar
			{
				render->SetTexture("Enemy2Walk.png");

				// Enemy2Walk.png: same layout as Pooka — 46×29, 2 rows × 3 columns
				// Row 0: walk right, Row 1: walk left, stride 15
				const std::string fWalkTex = "Enemy2Walk.png";
				constexpr int ECS = 15;
				animator->AddAnimation("walk_right", fWalkTex,
					{ {0, 0, EW, EH}, {ECS, 0, EW, EH}, {ECS * 2, 0, EW, EH} }, 6.f);
				animator->AddAnimation("walk_left", fWalkTex,
					{ {0, ECS, EW, EH}, {ECS, ECS, EW, EH}, {ECS * 2, ECS, EW, EH} }, 6.f);

				// Enemy2Ghost.png: 1 row × 2 columns, each frame 14×11, sheet 29×11
				const std::string fGhostTex = "Enemy2Ghost.png";
				constexpr int FGW = 14;
				constexpr int FGH = 11;
				animator->AddAnimation("ghost", fGhostTex,
					{ {0, 0, FGW, FGH}, {FGW + 1, 0, FGW, FGH} }, 6.f);
				animator->SetAnimationRenderSize("ghost", cellSize * 0.6f, cellSize * 0.6f);

				// Enemy2Fire.png is loaded separately for fire rendering in EnemyComponent

				// Inflate stages from individual sheets, faces left by default
				animator->AddAnimation("inflate_1", std::string("Enemy2Explode1.png"),
					{ {0, 0, 15, 13} }, 1.f, false);
				animator->AddAnimation("inflate_2", std::string("Enemy2Explode2.png"),
					{ {0, 0, 20, 19} }, 1.f, false);
				animator->AddAnimation("inflate_3", std::string("Enemy2Explode3.png"),
					{ {0, 0, 20, 21} }, 1.f, false);
				animator->AddAnimation("inflate_4", std::string("Enemy2Explode4.png"),
					{ {0, 0, 24, 23} }, 1.f, false);
			}

			animator->Play("walk_right");

			// canDig = false — enemies move through tunnels only
			float enemySpeed = (spawn.type == EnemySpawn::Type::Pooka) ? 2.5f : 2.0f;
			auto* movement = enemy->AddComponent<GridMovementComponent>(pGrid, enemySpeed, false);
			movement->SetGridPosition(spawn.gridX, spawn.gridY);

			// Ensure the spawn cell is passable so the enemy can move immediately
			pGrid->SetCellType(spawn.gridX, spawn.gridY, CellType::Tunnel);

			EnemyType eType = (spawn.type == EnemySpawn::Type::Pooka)
				? EnemyType::Pooka : EnemyType::Fygar;

			bool isVersusPlayer = (mode == GameMode::Versus &&
				spawn.type == EnemySpawn::Type::Fygar &&
				outVersusEnemy == nullptr);

			if (isVersusPlayer)
			{
				// Versus Fygar: has EnemyComponent for fire/inflate/scoring,
				// but is player-controlled (no AI chase logic)
				auto* enemyComp = enemy->AddComponent<EnemyComponent>(pGrid, eType, pPlayerTarget);
				enemyComp->SetPlayerControlled(true);
			}
			else
			{
				enemy->AddComponent<EnemyComponent>(pGrid, eType, pPlayerTarget);
			}

			GameObject* ptr = enemy.get();
			enemies.push_back(ptr);

			if (isVersusPlayer)
				outVersusEnemy = ptr;

			scene.Add(std::move(enemy));
		}

		return enemies;
	}

	std::vector<GameObject*> LevelLoader::CreateRocks(Scene& scene, GridComponent* pGrid,
		const LevelData& data,
		GameObject* pPlayer1, GameObject* pPlayer2,
		const std::vector<GameObject*>& enemies,
		ScoreComponent* pScore1, ScoreComponent* pScore2)
	{
		std::vector<GameObject*> rockPtrs;

		for (const auto& rockPos : data.rocks)
		{
			auto rock = std::make_unique<GameObject>();
			glm::vec2 pos = pGrid->GridToPixel(rockPos.gridX, rockPos.gridY);
			rock->AddComponent<TransformComponent>()->SetLocalPosition(pos.x, pos.y);

			auto* rockComp = rock->AddComponent<RockComponent>(
				pGrid, rockPos.gridX, rockPos.gridY);

			// Wire up player and enemy references
			if (pPlayer1)
				rockComp->AddPlayer(pPlayer1);
			if (pPlayer2)
				rockComp->AddPlayer(pPlayer2);

			for (auto* enemy : enemies)
				rockComp->AddEnemy(enemy);

			// Register score components as observers for rock crush events
			if (pScore1) rockComp->AddObserver(pScore1);
			if (pScore2) rockComp->AddObserver(pScore2);

			GameObject* ptr = rock.get();
			rockPtrs.push_back(ptr);
			scene.Add(std::move(rock));
		}

		return rockPtrs;
	}

	void LevelLoader::CreateHUD(Scene& scene, GameMode mode,
		ScoreComponent* pScore1, ScoreComponent* pScore2,
		GameObject* pPlayer1, GameObject* pPlayer2)
	{
		auto font = ResourceManager::GetInstance().LoadFont("Lingua.otf", 18);

		// Player 1 score display — observes ScoreComponent for live updates
		auto scoreGo = std::make_unique<GameObject>();
		scoreGo->AddComponent<TransformComponent>()->SetLocalPosition(10.f, 5.f);
		scoreGo->AddComponent<TextComponent>(font, "Score: 0");
		auto* scoreDisplay = scoreGo->AddComponent<PointsDisplayComponent>();
		if (pScore1)
			pScore1->AddObserver(scoreDisplay);
		scene.Add(std::move(scoreGo));

		// Player 1 lives display — observes PlayerCollisionComponent.
		// In co-op, lives are a shared pool shown by a single counter that
		// PlayingState manages, so skip the per-player display here.
		if (mode != GameMode::CoOp)
		{
			int p1Lives = 4;
			if (pPlayer1)
			{
				if (auto* c = pPlayer1->GetComponent<PlayerCollisionComponent>())
					p1Lives = c->GetLives();
			}

			auto livesGo = std::make_unique<GameObject>();
			livesGo->AddComponent<TransformComponent>()->SetLocalPosition(10.f, 25.f);
			livesGo->AddComponent<TextComponent>(font, "Lives: " + std::to_string(p1Lives));
			auto* livesDisplay = livesGo->AddComponent<LivesDisplayComponent>();
			if (pPlayer1)
			{
				auto* collision = pPlayer1->GetComponent<PlayerCollisionComponent>();
				if (collision)
					collision->AddObserver(livesDisplay);
			}
			scene.Add(std::move(livesGo));
		}

		auto roundGo = std::make_unique<GameObject>();
		roundGo->AddComponent<TransformComponent>()->SetLocalPosition(400.f, 5.f);
		roundGo->AddComponent<TextComponent>(font, "Round: 1");
		scene.Add(std::move(roundGo));

		if (mode == GameMode::CoOp)
		{
			// Player 2 score display
			auto score2Go = std::make_unique<GameObject>();
			score2Go->AddComponent<TransformComponent>()->SetLocalPosition(500.f, 5.f);
			score2Go->AddComponent<TextComponent>(font, "P2 Score: 0");
			auto* score2Display = score2Go->AddComponent<PointsDisplayComponent>("P2 Score: ");
			if (pScore2)
				pScore2->AddObserver(score2Display);
			scene.Add(std::move(score2Go));

			// Lives are shared in co-op — PlayingState draws the single counter.
		}

		if (mode == GameMode::Versus)
		{
			auto versusGo = std::make_unique<GameObject>();
			versusGo->AddComponent<TransformComponent>()->SetLocalPosition(250.f, 450.f);
			versusGo->AddComponent<TextComponent>(font, "VERSUS MODE");
			scene.Add(std::move(versusGo));
		}
	}

	std::vector<GameObject*> LevelLoader::RespawnEnemies(Scene& scene, GridComponent* pGrid,
		const LevelData& data, GameMode mode,
		GameObject* pPlayer1, GameObject* pPlayer2,
		ScoreComponent* pScore1, ScoreComponent* pScore2)
	{
		GameObject* versusEnemy = nullptr;
		auto enemies = CreateEnemies(scene, pGrid, data, mode, versusEnemy, pPlayer1);

		// Rewire player collision and pump to new enemies
		auto wirePlayer = [&](GameObject* pPlayer)
		{
			if (!pPlayer) return;

			auto* collision = pPlayer->GetComponent<PlayerCollisionComponent>();
			if (collision)
			{
				collision->ClearEnemies();
				for (auto* enemy : enemies)
					collision->AddEnemy(enemy);
			}

			auto* pump = pPlayer->GetComponent<PumpComponent>();
			if (pump)
			{
				pump->ClearEnemies();
				for (auto* enemy : enemies)
					pump->AddEnemy(enemy);
			}
		};

		wirePlayer(pPlayer1);
		wirePlayer(pPlayer2);

		// Register score observers on new enemies
		auto wireScore = [&](ScoreComponent* pScore)
		{
			if (!pScore) return;
			for (auto* enemy : enemies)
			{
				auto* enemyComp = enemy->GetComponent<EnemyComponent>();
				if (enemyComp)
					enemyComp->AddObserver(pScore);
			}
		};

		wireScore(pScore1);
		wireScore(pScore2);

		// Add Player 2 as a target so respawned enemies chase the closest player
		if (pPlayer2)
		{
			for (auto* enemy : enemies)
			{
				auto* enemyComp = enemy->GetComponent<EnemyComponent>();
				if (enemyComp)
					enemyComp->AddTarget(pPlayer2);
			}
		}

		// Wire rock observers to new enemies
		// (rocks don't need rewiring — they check enemies by proximity each frame)

		return enemies;
	}
}