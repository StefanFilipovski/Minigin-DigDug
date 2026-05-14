#include "ScoreComponent.h"
#include "EnemyComponent.h"
#include "RockComponent.h"
#include "TransformComponent.h"
#include "TextComponent.h"
#include "FloatingScoreComponent.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "GameObject.h"
#include "GameEventIds.h"
#include <string>

namespace dae
{
	ScoreComponent::ScoreComponent(GameObject* pOwner)
		: Component(pOwner)
	{
	}

	void ScoreComponent::AddPoints(int points)
	{
		m_Score += points;
		NotifyObservers(EVENT_POINTS_GAINED, GetOwner());
	}

	void ScoreComponent::Notify(EventId event, GameObject* pGameObject)
	{
		if (!pGameObject) return;

		if (event == EVENT_ENEMY_KILLED)
			HandleEnemyKilled(pGameObject);
		else if (event == EVENT_ROCK_CRUSH_COMPLETE)
			HandleRockCrushComplete(pGameObject);
	}

	void ScoreComponent::HandleEnemyKilled(GameObject* pEnemyGO)
	{
		auto* enemy = pEnemyGO->GetComponent<EnemyComponent>();
		if (!enemy) return;

		// Rock crushes are scored via EVENT_ROCK_CRUSH_COMPLETE
		if (enemy->WasCrushedByRock()) return;

		// Pump kill — award layer-based points (Fygar horizontal bonus
		// is already factored into GetScoreValue)
		int points = enemy->GetScoreValue();
		AddPoints(points);

		// Spawn floating popup at the enemy's position
		auto* enemyTransform = pEnemyGO->GetComponent<TransformComponent>();
		if (enemyTransform)
		{
			const auto& pos = enemyTransform->GetWorldPosition();
			SpawnScorePopup(points, pos.x, pos.y);
		}
	}

	void ScoreComponent::HandleRockCrushComplete(GameObject* pRockGO)
	{
		auto* rock = pRockGO->GetComponent<RockComponent>();
		if (!rock) return;

		int crushCount = rock->GetCrushCount();
		if (crushCount <= 0) return;

		int points = GetRockCrushScore(crushCount);
		AddPoints(points);

		// Spawn popup at the rock's landing position
		auto* rockTransform = pRockGO->GetComponent<TransformComponent>();
		if (rockTransform)
		{
			const auto& pos = rockTransform->GetWorldPosition();
			SpawnScorePopup(points, pos.x, pos.y);
		}
	}

	void ScoreComponent::SpawnScorePopup(int points, float x, float y)
	{
		if (!m_pScene) return;

		// Load a small font for popups if none set
		if (!m_popupFont)
			m_popupFont = ResourceManager::GetInstance().LoadFont("Lingua.otf", 14);

		auto popup = std::make_unique<GameObject>();
		popup->AddComponent<TransformComponent>()->SetLocalPosition(x, y);

		auto* text = popup->AddComponent<TextComponent>(m_popupFont, std::to_string(points));
		text->SetColor(SDL_Color{ 0, 150, 255, 255 }); // blue

		popup->AddComponent<FloatingScoreComponent>(1.2f, 40.f);

		m_pScene->Add(std::move(popup));
	}

	int ScoreComponent::GetRockCrushScore(int enemyCount)
	{
		// Dig Dug rock crush bonus table
		switch (enemyCount)
		{
		case 1:  return 1000;
		case 2:  return 2500;
		case 3:  return 4000;
		case 4:  return 6000;
		case 5:  return 8000;
		case 6:  return 10000;
		case 7:  return 12000;
		case 8:  return 15000;
		default: return 15000; // cap at 8+
		}
	}
}
