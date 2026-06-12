#include "PumpHoseComponent.h"
#include "GridComponent.h"
#include "TransformComponent.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "Texture2D.h"
#include "GameObject.h"
#include <SDL3/SDL.h>
#include <algorithm>

namespace dae
{
	PumpHoseComponent::PumpHoseComponent(GameObject* pOwner, GridComponent* pGrid)
		: Component(pOwner)
		, m_pGrid(pGrid)
	{
		// Red background gets keyed out so the rope renders cleanly
		constexpr SDL_Color RopeColorKey{ 108, 7, 0, 255 };
		m_hoseTexture = ResourceManager::GetInstance().LoadTexture("PumpString.png", RopeColorKey);

		float cs = static_cast<float>(pGrid->GetCellSize());
		float scale = cs / 16.f;
		m_hoseRenderLong = m_hoseSrcW * scale;
		m_hoseRenderShort = m_hoseSrcH * scale;
	}

	void PumpHoseComponent::Show(const glm::ivec2& direction, float lengthCells, int rangeCells)
	{
		m_visible = true;
		m_direction = direction;
		m_lengthCells = lengthCells;
		m_rangeCells = rangeCells;
	}

	void PumpHoseComponent::Render() const
	{
		if (!m_visible) return;
		if (m_lengthCells <= 0.f) return;
		if (!m_hoseTexture) return;

		auto* transform = GetOwner()->GetComponent<TransformComponent>();
		if (!transform) return;

		
		const auto& pos = transform->GetWorldPosition();
		float cs = static_cast<float>(m_pGrid->GetCellSize());
		auto& renderer = Renderer::GetInstance();

		float spriteCellsLong = m_hoseRenderLong / cs;
		float clampedLength = std::min(m_lengthCells, spriteCellsLong);
		clampedLength = std::min(clampedLength, static_cast<float>(m_rangeCells));

		float fraction = clampedLength / spriteCellsLong;
		if (fraction <= 0.f) return;
		if (fraction > 1.f) fraction = 1.f;

		float visibleLong = m_hoseRenderLong * fraction;
		float visibleSrcW = m_hoseSrcW * fraction;

		SDL_FRect src{ 0.f, 0.f, visibleSrcW, m_hoseSrcH };

	
		float anchorCx = pos.x + cs * 0.5f;
		float anchorCy = pos.y + cs * 0.5f;

		
		double angle = 0.0;
		if (m_direction.x > 0)      angle = 0.0;
		else if (m_direction.y > 0) angle = 90.0;
		else if (m_direction.x < 0) angle = 180.0;
		else if (m_direction.y < 0) angle = 270.0;

		SDL_FPoint pivot{ 0.f, m_hoseRenderShort * 0.5f };

		float startOffset = cs * 0.5f;
		float dstX = anchorCx - pivot.x + static_cast<float>(m_direction.x) * startOffset;
		float dstY = anchorCy - pivot.y + static_cast<float>(m_direction.y) * startOffset;

		renderer.RenderTextureRotated(*m_hoseTexture, dstX, dstY,
			src, visibleLong, m_hoseRenderShort, angle, &pivot);
	}
}
