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

namespace dae
{
	PumpComponent::PumpComponent(GameObject* pOwner, GridComponent* pGrid)
		: Component(pOwner)
		, m_pGrid(pGrid)
	{
		m_spriteSheet = ResourceManager::GetInstance().LoadTexture("general_sprites.png");

		float cs = static_cast<float>(pGrid->GetCellSize());

		// Hose sprites — rendered at natural proportions scaled to grid cell size
		// The original sprites are in a 16px grid, so we scale by (cellSize / 16)
		float scale = cs / 16.f;

		// Right hose: 32x15 on sheet → 2 cells wide, ~1 cell tall at natural size
		m_hoseRight.srcRect = { 32.f, 48.f, 32.f, 15.f };
		m_hoseRight.renderWidth = 32.f * scale;
		m_hoseRight.renderHeight = 15.f * scale;

		// Left hose: 32x6 on sheet
		m_hoseLeft.srcRect = { 96.f, 56.f, 32.f, 6.f };
		m_hoseLeft.renderWidth = 32.f * scale;
		m_hoseLeft.renderHeight = 6.f * scale;

		// Down hose: 6x32 on sheet (swapped — was at x=8, now x=72)
		m_hoseDown.srcRect = { 72.f, 48.f, 6.f, 32.f };
		m_hoseDown.renderWidth = 6.f * scale;
		m_hoseDown.renderHeight = 32.f * scale;

		// Up hose: 6x32 on sheet (swapped — was at x=72, now x=8)
		m_hoseUp.srcRect = { 8.f, 48.f, 6.f, 32.f };
		m_hoseUp.renderWidth = 6.f * scale;
		m_hoseUp.renderHeight = 32.f * scale;
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
		if (!m_spriteSheet) return;

		auto* transform = GetOwner()->GetComponent<TransformComponent>();
		if (!transform) return;

		const auto& pos = transform->GetWorldPosition();
		float cs = static_cast<float>(m_pGrid->GetCellSize());
		auto& renderer = Renderer::GetInstance();

		const HoseSprite* hose = nullptr;
		if (m_fireDirection.x > 0) hose = &m_hoseRight;
		else if (m_fireDirection.x < 0) hose = &m_hoseLeft;
		else if (m_fireDirection.y < 0) hose = &m_hoseUp;
		else if (m_fireDirection.y > 0) hose = &m_hoseDown;

		if (!hose) return;

		// The hose sprite has a natural render size (renderWidth x renderHeight).
		// m_hoseLength is how far the hose has extended in grid cells.
		// The hose sprite covers a fixed number of cells at its natural size.
		// We animate it by revealing it progressively (clipping the source rect).

		// How many cells the full sprite covers
		float spriteCellsWide = hose->renderWidth / cs;
		float spriteCellsTall = hose->renderHeight / cs;

		// How much of the sprite to show (0.0 to 1.0)
		float hoseCellLength; // how many cells the hose should cover right now
		if (m_fireDirection.x != 0)
			hoseCellLength = spriteCellsWide;
		else
			hoseCellLength = spriteCellsTall;

		// Clamp to the available range (wall distance)
		float maxShow = static_cast<float>(m_currentRange);
		float clampedLength = (m_hoseLength < hoseCellLength) ? m_hoseLength : hoseCellLength;
		if (clampedLength > maxShow) clampedLength = maxShow;

		float fraction = clampedLength / hoseCellLength;
		if (fraction > 1.f) fraction = 1.f;
		if (fraction <= 0.f) return;

		SDL_FRect src = hose->srcRect;
		float dstX, dstY, dstW, dstH;

		if (m_fireDirection.x > 0) // Right
		{
			// Reveal from left to right
			src.w = hose->srcRect.w * fraction;
			dstX = pos.x + cs;
			dstY = pos.y + (cs - hose->renderHeight) * 0.5f; // Center vertically
			dstW = hose->renderWidth * fraction;
			dstH = hose->renderHeight;
		}
		else if (m_fireDirection.x < 0) // Left
		{
			// Reveal from right to left
			float clipW = hose->srcRect.w * fraction;
			src.x = hose->srcRect.x + hose->srcRect.w - clipW;
			src.w = clipW;
			dstW = hose->renderWidth * fraction;
			dstH = hose->renderHeight;
			dstX = pos.x - dstW;
			dstY = pos.y + (cs - hose->renderHeight) * 0.5f;
		}
		else if (m_fireDirection.y < 0) // Up
		{
			// Reveal from bottom to top
			float clipH = hose->srcRect.h * fraction;
			src.y = hose->srcRect.y + hose->srcRect.h - clipH;
			src.h = clipH;
			dstW = hose->renderWidth;
			dstH = hose->renderHeight * fraction;
			dstX = pos.x + (cs - hose->renderWidth) * 0.5f; // Center horizontally
			dstY = pos.y - dstH;
		}
		else // Down
		{
			// Reveal from top to bottom
			src.h = hose->srcRect.h * fraction;
			dstX = pos.x + (cs - hose->renderWidth) * 0.5f;
			dstY = pos.y + cs;
			dstW = hose->renderWidth;
			dstH = hose->renderHeight * fraction;
		}

		renderer.RenderTexture(*m_spriteSheet, dstX, dstY, src, dstW, dstH);
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