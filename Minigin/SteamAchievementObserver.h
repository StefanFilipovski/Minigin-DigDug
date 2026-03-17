#pragma once
#include "Component.h"
#include "Observer.h"
#include "ScoreComponent.h"
#include "GameObject.h"
#include "EventIds.h"

#ifdef USE_STEAMWORKS
#include "steam_api.h"
#endif

namespace dae
{

	// Implemented as a Component so its lifetime is managed by its owner GameObject.
	// Implemented as an Observer so it has no coupling to the game objects it watches.
	class SteamAchievementObserver final : public Component, public Observer
	{
	public:
		explicit SteamAchievementObserver(GameObject* pOwner)
			: Component(pOwner) {
		}



		void Notify(EventId event, GameObject* pGameObject) override
		{
			if (event != EVENT_POINTS_GAINED) return;
			if (!pGameObject) return;



			auto* pScore = pGameObject->GetComponent<ScoreComponent>();
			if (!pScore) return;


			if (pScore->GetScore() >= 500)
				UnlockWinnerAchievement();
		}

	private:

		void UnlockWinnerAchievement()
		{
			if (m_Unlocked) return;
			m_Unlocked = true;

#ifdef USE_STEAMWORKS
			SteamUserStats()->SetAchievement("ACH_WIN_ONE_GAME");
			SteamUserStats()->StoreStats();
#endif
		}

		bool m_Unlocked{ false };
	};
}