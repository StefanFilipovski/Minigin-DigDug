#pragma once
#include "Component.h"
#include "Subject.h"
#include "Observer.h"
#include <memory>

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

		void SpawnScorePopup(int points, float x, float y);
		void HandleEnemyKilled(GameObject* pEnemyGO);
		void HandleRockCrushComplete(GameObject* pRockGO);

		static int GetRockCrushScore(int enemyCount);
	};
}
