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
		auto& rm = ResourceManager::GetInstance();
		m_dirtTextures[0] = rm.LoadTexture("yellow_01.png");
		m_dirtTextures[1] = rm.LoadTexture("ground_08.png");
		m_dirtTextures[2] = rm.LoadTexture("ground_16.png");
		m_dirtTextures[3] = rm.LoadTexture("ground_24.png");
		m_tunnelTexture = rm.LoadTexture("tunnel.png");
		m_surfaceTexture = rm.LoadTexture("sky.png");
	}

	void GridComponent::Render() const
	{
		auto& renderer = Renderer::GetInstance();
		float cs = static_cast<float>(m_cellSize);

		for (int y = 0; y < m_height; ++y)
		{
			int layer = GetLayer(y);
			int texIdx = std::clamp(layer - 1, 0, 3);

			for (int x = 0; x < m_width; ++x)
			{
				const auto& cell = m_cells[Index(x, y)];
				float px = static_cast<float>(m_gridOffset.x + x * m_cellSize);
				float py = static_cast<float>(m_gridOffset.y + y * m_cellSize);

				switch (cell.type)
				{
				case CellType::Dirt:
				{
					if (m_dirtTextures[texIdx])
						renderer.RenderTexture(*m_dirtTextures[texIdx], px, py, cs, cs);
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
		// Surface rows are layer 1 (yellow)
		if (gridY < m_surfaceRows) return 1;

		// Underground rows split evenly into 4 layers
		int underground = gridY - m_surfaceRows;
		int undergroundTotal = m_height - m_surfaceRows;
		int rowsPerLayer = undergroundTotal / 4;
		if (rowsPerLayer < 1) rowsPerLayer = 1;

		int layer = (underground / rowsPerLayer) + 1;
		if (layer > 4) layer = 4;
		return layer;
	}
}