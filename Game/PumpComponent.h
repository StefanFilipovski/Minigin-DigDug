#pragma once
#include "Component.h"
#include <memory>
#include <vector>
#include <SDL3/SDL.h>
#include <glm/ext/vector_int2.hpp>

namespace dae
{
	class GridComponent;
	class GridMovementComponent;
	class SpriteAnimatorComponent;
	class EnemyComponent;
	class PlayerCollisionComponent;
	class RenderComponent;
	class Texture2D;
	class GameObject;

	class PumpComponent final : public Component
	{
	public:
		PumpComponent(GameObject* pOwner, GridComponent* pGrid);

		void Update(float deltaTime) override;
		void Render() const override;

		// Called by the pump command
		void Fire();

		void AddEnemy(GameObject* pEnemy);
		void ClearEnemies();

		bool IsFiring() const { return m_state != PumpState::Idle; }
		bool IsLatched() const { return m_state == PumpState::Latched; }

		void SetRange(int cells) { m_maxRange = cells; }
		void ForceReset();

	private:
		enum class PumpState
		{
			Idle,
			Extending,
			Latched,
			Retracting
		};

		GridComponent* m_pGrid;
		GridMovementComponent* m_pMovement{ nullptr };
		SpriteAnimatorComponent* m_pAnimator{ nullptr };
		PlayerCollisionComponent* m_pCollision{ nullptr };
		bool m_cached{ false };

		std::vector<GameObject*> m_enemies;
		GameObject* m_pLatchedEnemyGO{ nullptr };

		PumpState m_state{ PumpState::Idle };
		glm::ivec2 m_fireDirection{ 0, 0 };

		float m_hoseLength{ 0.f };
		int m_maxRange{ 4 };
		int m_currentRange{ 0 };
		float m_extendSpeed{ 12.f };
		float m_retractSpeed{ 16.f };

		float m_retractTimer{ 0.f };
		static constexpr float RetractDelay{ 0.15f };

		// Hold-to-pump
		bool  m_fireButtonHeld{ false };      
		bool  m_fireButtonHeldPrev{ false };  
		float m_holdPumpTimer{ 0.f };         
		static constexpr float HoldPumpInterval{ 0.4f };  

		
		
		std::shared_ptr<Texture2D> m_hoseTexture;
		float m_hoseSrcW{ 32.f }; 
		float m_hoseSrcH{ 6.f };  
		float m_hoseRenderLong{ 32.f };  
		float m_hoseRenderShort{ 6.f };  

		void CalculateRange();
		void CacheComponents();
	};
}