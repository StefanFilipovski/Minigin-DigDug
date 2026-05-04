#include "PumpComponent.h"
#include "GridComponent.h"
#include "GridMovementComponent.h"
#include "SpriteAnimatorComponent.h"
#include "RenderComponent.h"
#include "TransformComponent.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "Texture2D.h"
#include "GameObject.h"
#include <algorithm>

namespace dae
{
	PumpComponent::PumpComponent(GameObject* pOwner, GridComponent* pGrid)
		: Component(pOwner)
		, m_pGrid(pGrid)
	{
		// Red background gets keyed out so the rope renders cleanly
		SDL_Color redKey{ 255, 0, 0, 255 };
		m_hoseTexture = ResourceManager::GetInstance().LoadTexture("PumpString.png", redKey);

		// Native sprite is 32×6 (right-facing). Scale it to grid cell size:
		// the long axis spans 2 cells (32px = 2 × 16px native), the short axis is 6/16 of a cell.
		float cs = static_cast<float>(pGrid->GetCellSize());
		float scale = cs / 16.f;
		m_hoseRenderLong = m_hoseSrcW * scale;
		m_hoseRenderShort = m_hoseSrcH * scale;
	}

	void PumpComponent::CacheComponents()
	{
		if (m_cached) return;
		m_pMovement = GetOwner()->GetComponent<GridMovementComponent>();
		m_pAnimator = GetOwner()->GetComponent<SpriteAnimatorComponent>();
		m_cached = true;
	}

	void PumpComponent::Fire()
	{
		CacheComponents();
		if (!m_pMovement) return;
		if (m_pMovement->IsMoving()) return;

		if (m_state == PumpState::Idle)
		{
			m_fireDirection = m_pMovement->GetCurrentDirection();
			if (m_fireDirection == glm::ivec2{ 0, 0 })
				m_fireDirection = { 1, 0 };

			m_hoseLength = 0.f;
			m_state = PumpState::Extending;

			CalculateRange();

			if (m_pAnimator)
			{
				if (m_fireDirection.x > 0) m_pAnimator->Play("pump_right");
				else if (m_fireDirection.x < 0) m_pAnimator->Play("pump_left");
				else if (m_fireDirection.y < 0) m_pAnimator->Play("pump_up");
				else if (m_fireDirection.y > 0) m_pAnimator->Play("pump_down");
			}
		}
	}

	void PumpComponent::Update(float deltaTime)
	{
		CacheComponents();

		switch (m_state)
		{
		case PumpState::Idle:
			break;

		case PumpState::Extending:
		{
			m_hoseLength += m_extendSpeed * deltaTime;

			if (m_hoseLength >= static_cast<float>(m_currentRange))
			{
				m_hoseLength = static_cast<float>(m_currentRange);
				m_state = PumpState::Retracting;
				m_retractTimer = RetractDelay;
			}
			break;
		}

		case PumpState::Latched:
			break;

		case PumpState::Retracting:
		{
			m_retractTimer -= deltaTime;
			if (m_retractTimer > 0.f)
				break;

			m_hoseLength -= m_retractSpeed * deltaTime;

			if (m_hoseLength <= 0.f)
			{
				m_hoseLength = 0.f;
				m_state = PumpState::Idle;

				if (m_pAnimator)
				{
					if (m_fireDirection.x > 0) m_pAnimator->Play("walk_right");
					else if (m_fireDirection.x < 0) m_pAnimator->Play("walk_left");
					else if (m_fireDirection.y < 0) m_pAnimator->Play("walk_up");
					else if (m_fireDirection.y > 0) m_pAnimator->Play("walk_down");
					m_pAnimator->Pause();
				}
			}
			break;
		}
		}
	}

	void PumpComponent::Render() const
	{
		if (m_state == PumpState::Idle) return;
		if (m_hoseLength <= 0.f) return;
		if (!m_hoseTexture) return;

		auto* transform = GetOwner()->GetComponent<TransformComponent>();
		if (!transform) return;

		const auto& pos = transform->GetWorldPosition();
		float cs = static_cast<float>(m_pGrid->GetCellSize());
		auto& renderer = Renderer::GetInstance();

		// Extension fraction: the sprite's full length covers (m_hoseRenderLong / cs) cells.
		// Reveal progressively by clipping the source rect from the player-end inward.
		float spriteCellsLong = m_hoseRenderLong / cs;
		float clampedLength = std::min(m_hoseLength, spriteCellsLong);
		clampedLength = std::min(clampedLength, static_cast<float>(m_currentRange));

		float fraction = clampedLength / spriteCellsLong;
		if (fraction <= 0.f) return;
		if (fraction > 1.f) fraction = 1.f;

		float visibleLong = m_hoseRenderLong * fraction;
		float visibleSrcW = m_hoseSrcW * fraction;

		// Source rect: native sprite extends from the player rightward, so
		// reveal from left edge of the source. Rotation handles the visual flip.
		SDL_FRect src{ 0.f, 0.f, visibleSrcW, m_hoseSrcH };

		// Player-cell anchor in screen space (the cell the player occupies).
		float playerCx = pos.x + cs * 0.5f;
		float playerCy = pos.y + cs * 0.5f;

		// We always draw the sprite as a horizontal bar of size (visibleLong × short),
		// then rotate it so that its "left edge" sits at the player center and its
		// "right edge" reaches outward in the firing direction.
		double angle = 0.0;
		if (m_fireDirection.x > 0)      angle = 0.0;    // right
		else if (m_fireDirection.y > 0) angle = 90.0;   // down
		else if (m_fireDirection.x < 0) angle = 180.0;  // left
		else if (m_fireDirection.y < 0) angle = 270.0;  // up

		// Rotation pivot: the player-side end of the bar (left-middle in unrotated space).
		SDL_FPoint pivot{ 0.f, m_hoseRenderShort * 0.5f };

		// Position so that (pivot) lands on the player center, offset by a half-cell along
		// the firing direction so the rope visually starts at the cell edge, not the middle.
		float startOffset = cs * 0.5f;
		float dstX = playerCx - pivot.x + static_cast<float>(m_fireDirection.x) * startOffset;
		float dstY = playerCy - pivot.y + static_cast<float>(m_fireDirection.y) * startOffset;

		renderer.RenderTextureRotated(*m_hoseTexture, dstX, dstY,
			src, visibleLong, m_hoseRenderShort, angle, &pivot);
	}

	void PumpComponent::CalculateRange()
	{
		if (!m_pMovement || !m_pGrid) return;

		const auto& gridPos = m_pMovement->GetGridPosition();
		m_currentRange = 0;

		for (int i = 1; i <= m_maxRange; ++i)
		{
			glm::ivec2 checkPos = gridPos + m_fireDirection * i;

			if (!m_pGrid->IsInBounds(checkPos.x, checkPos.y))
				break;

			CellType cellType = m_pGrid->GetCellType(checkPos.x, checkPos.y);

			if (cellType == CellType::Dirt)
				break;

			m_currentRange = i;
		}

		if (m_currentRange == 0)
			m_currentRange = 1;
	}
}