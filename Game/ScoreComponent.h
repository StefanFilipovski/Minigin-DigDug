#pragma once
#include "Component.h"
#include "Subject.h"
#include "Observer.h"
#include <memory>
#include <vector>

namespace dae
{
	class Scene;
	class Font;

	class ScoreComponent final : public Component, public Subject, public Observer
	{
	public:
		explicit ScoreComponent(GameObject* pOwner);

		void AddPoints(int points);
		int  GetScore() const { return m_Score; }

		// Restore a score carried over from the previous level (players are
		// rebuilt on level load, so the running total lives in GameSession)
		void SetScore(int score);

		// Set the scene used to spawn floating score popups
		void SetScene(Scene* pScene) { m_pScene = pScene; }
		void SetPopupFont(std::shared_ptr<Font> font) { m_popupFont = font; }

		// Observer — receives EVENT_ENEMY_KILLED from enemies
		//           and EVENT_ROCK_CRUSH_COMPLETE from rocks
		void Notify(EventId event, GameObject* pGameObject) override;

	private:
		int m_Score{ 0 };
		Scene* m_pScene{ nullptr };
		std::shared_ptr<Font> m_popupFont;

		// Object pool for score popups — popups deactivate instead of being
		// destroyed, and spawning reuses an inactive one
		static constexpr size_t MaxPopups{ 8 };
		std::vector<GameObject*> m_popupPool;

		void SpawnScorePopup(int points, float x, float y);
		void HandleEnemyKilled(GameObject* pEnemyGO);
		void HandleRockCrushComplete(GameObject* pRockGO);

		static int GetRockCrushScore(int enemyCount);
	};
}
