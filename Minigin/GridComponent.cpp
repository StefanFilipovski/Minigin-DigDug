#include "GridComponent.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "Texture2D.h"
#include <cassert>
#include <algorithm>

namespace dae
{
	GridComponent::GridComponent(GameObject* pOwner, int width, int height,
		int cellSize, const glm::ivec2& gridOffset)
		: Component(pOwner)
		, m_width(width)
		, m_height(height)
		, m_cellSize(cellSize)
		, m_gridOffset(gridOffset)
		, m_cells(static_cast<size_t>(width* height))
	{
		// Load the placeholder textures for grid rendering
		m_dirtTexture = ResourceManager::GetInstance().LoadTexture("dirt.png");
		m_tunnelTexture = ResourceManager::GetInstance().LoadTexture("tunnel.png");
		m_surfaceTexture = ResourceManager::GetInstance().LoadTexture("surface.png");
	}

	void GridComponent::Render() const
	{
		auto& renderer = Renderer::GetInstance();
		float cs = static_cast<float>(m_cellSize);

		for (int y = 0; y < m_height; ++y)
		{
			for (int x = 0; x < m_width; ++x)
			{
				const auto& cell = m_cells[Index(x, y)];
				float px = static_cast<float>(m_gridOffset.x + x * m_cellSize);
				float py = static_cast<float>(m_gridOffset.y + y * m_cellSize);

				switch (cell.type)
				{
				case CellType::Dirt:
				{
					// Draw dirt as base
					if (m_dirtTexture)
						renderer.RenderTexture(*m_dirtTexture, px, py, cs, cs);

					// If partially dug, draw tunnel over the dug portions
					// For now, just show dirt — partial digging is an advanced visual
					// that we can add later with the actual tunnel edge sprites
					break;
				}
				case CellType::Tunnel:
					if (m_tunnelTexture)
						renderer.RenderTexture(*m_tunnelTexture, px, py, cs, cs);
					break;
				case CellType::Surface:
					if (m_surfaceTexture)
						renderer.RenderTexture(*m_surfaceTexture, px, py, cs, cs);
					break;
				}
			}
		}
	}

	CellData& GridComponent::GetCell(int gridX, int gridY)
	{
		assert(IsInBounds(gridX, gridY));
		return m_cells[Index(gridX, gridY)];
	}

	const CellData& GridComponent::GetCell(int gridX, int gridY) const
	{
		assert(IsInBounds(gridX, gridY));
		return m_cells[Index(gridX, gridY)];
	}

	CellType GridComponent::GetCellType(int gridX, int gridY) const
	{
		if (!IsInBounds(gridX, gridY))
			return CellType::Dirt;
		return m_cells[Index(gridX, gridY)].type;
	}

	void GridComponent::DigCell(int gridX, int gridY, const glm::ivec2& moveDirection)
	{
		if (!IsInBounds(gridX, gridY)) return;

		auto& cell = m_cells[Index(gridX, gridY)];

		if (cell.type == CellType::Surface || cell.type == CellType::Tunnel)
			return;

		// When the player digs through a dirt cell, it becomes a tunnel immediately.
		// The partial digging (open edges) is tracked for enemy pathfinding,
		// but visually we just switch to tunnel for now.
		cell.type = CellType::Tunnel;
		cell.openLeft = true;
		cell.openRight = true;
		cell.openUp = true;
		cell.openDown = true;

		(void)moveDirection; // Will be used for partial dig visuals later
	}

	void GridComponent::SetCellType(int gridX, int gridY, CellType type)
	{
		if (!IsInBounds(gridX, gridY)) return;

		auto& cell = m_cells[Index(gridX, gridY)];
		cell.type = type;

		if (type == CellType::Tunnel || type == CellType::Surface)
		{
			cell.openLeft = true;
			cell.openRight = true;
			cell.openUp = true;
			cell.openDown = true;
		}
	}

	bool GridComponent::IsInBounds(int gridX, int gridY) const
	{
		return gridX >= 0 && gridX < m_width && gridY >= 0 && gridY < m_height;
	}

	glm::vec2 GridComponent::GridToPixel(int gridX, int gridY) const
	{
		return {
			static_cast<float>(m_gridOffset.x + gridX * m_cellSize),
			static_cast<float>(m_gridOffset.y + gridY * m_cellSize)
		};
	}

	glm::vec2 GridComponent::GridToPixelCenter(int gridX, int gridY) const
	{
		float halfCell = static_cast<float>(m_cellSize) * 0.5f;
		return {
			static_cast<float>(m_gridOffset.x + gridX * m_cellSize) + halfCell,
			static_cast<float>(m_gridOffset.y + gridY * m_cellSize) + halfCell
		};
	}

	glm::ivec2 GridComponent::PixelToGrid(float pixelX, float pixelY) const
	{
		int gx = static_cast<int>((pixelX - m_gridOffset.x) / m_cellSize);
		int gy = static_cast<int>((pixelY - m_gridOffset.y) / m_cellSize);
		return { std::clamp(gx, 0, m_width - 1), std::clamp(gy, 0, m_height - 1) };
	}

	int GridComponent::GetLayer(int gridY) const
	{
		if (gridY <= 4) return 1;
		if (gridY <= 7) return 2;
		if (gridY <= 10) return 3;
		return 4;
	}
}