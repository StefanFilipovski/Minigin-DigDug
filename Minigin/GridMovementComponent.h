#pragma once
#include "Component.h"
#include "../out/build/x64-debug/_deps/glm-src/glm/ext/vector_int2.hpp"
#include "../out/build/x64-debug/_deps/glm-src/glm/ext/vector_float2.hpp"

namespace dae
{
	class GridComponent;
	class TransformComponent;

	class GridMovementComponent final : public Component
	{
	public:
		GridMovementComponent(GameObject* pOwner, GridComponent* pGrid,
			float speed, bool canDig = false);

		void Update(float deltaTime) override;
		void FixedUpdate(float fixedTimeStep) override;

		// Must be called every frame the key is held (KeyState::Pressed)
		void SetDesiredDirection(const glm::ivec2& direction);

		const glm::ivec2& GetGridPosition() const { return m_gridPos; }
		const glm::ivec2& GetCurrentDirection() const { return m_currentDirection; }
		bool IsMoving() const { return m_isMoving; }

		void SetGridPosition(int gridX, int gridY);

		void SetGhostMode(bool ghost) { m_isGhost = ghost; }
		bool IsGhostMode() const { return m_isGhost; }

		void SetSpeed(float speed) { m_speed = speed; }
		float GetSpeed() const { return m_speed; }

		// True if the player is currently moving into a cell that was dirt
		bool IsDigging() const { return m_isDigging; }

	private:
		bool CanMoveTo(const glm::ivec2& targetCell, const glm::ivec2& direction) const;
		void ArriveAtCell();

		GridComponent* m_pGrid;
		TransformComponent* m_pTransform{ nullptr };

		glm::ivec2 m_gridPos{ 0, 0 };
		glm::ivec2 m_targetGridPos{ 0, 0 };

		glm::ivec2 m_currentDirection{ 0, 0 };
		glm::ivec2 m_desiredDirection{ 0, 0 };

		glm::vec2 m_pixelPos{ 0.f, 0.f };
		glm::vec2 m_startPixel{ 0.f, 0.f };
		glm::vec2 m_targetPixel{ 0.f, 0.f };
		float m_moveProgress{ 0.f };

		float m_speed;
		bool m_canDig;
		bool m_isGhost{ false };
		bool m_isMoving{ false };
		bool m_inputActive{ false };
		bool m_isDigging{ false };
	};
}