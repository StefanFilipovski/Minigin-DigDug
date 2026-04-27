#pragma once
#include "Component.h"
#include <vector>
#include <memory>
#include "../out/build/x64-debug/_deps/glm-src/glm/ext/vector_int2.hpp"
#include "../out/build/x64-debug/_deps/glm-src/glm/ext/vector_float2.hpp"

namespace dae
{
	class Texture2D;

	enum class CellType : uint8_t
	{
		Dirt,
		Tunnel,
		Surface
	};

	struct CellData
	{
		CellType type{ CellType::Dirt };

		bool openLeft{ false };
		bool openRight{ false };
		bool openUp{ false };
		bool openDown{ false };

		bool IsFullyDug() const
		{
			return type == CellType::Tunnel || type == CellType::Surface;
		}

		bool IsPassableFrom(const glm::ivec2& direction) const
		{
			if (type == CellType::Tunnel || type == CellType::Surface)
				return true;

			if (direction.x > 0)  return openLeft;
			if (direction.x < 0)  return openRight;
			if (direction.y > 0)  return openUp;
			if (direction.y < 0)  return openDown;

			return false;
		}
	};

	class GridComponent final : public Component
	{
	public:
		GridComponent(GameObject* pOwner, int width, int height, int cellSize,
			const glm::ivec2& gridOffset);

		void Render() const override;

		CellData& GetCell(int gridX, int gridY);
		const CellData& GetCell(int gridX, int gridY) const;
		CellType GetCellType(int gridX, int gridY) const;

		void DigCell(int gridX, int gridY, const glm::ivec2& moveDirection);
		void SetCellType(int gridX, int gridY, CellType type);

		bool IsInBounds(int gridX, int gridY) const;

		glm::vec2 GridToPixel(int gridX, int gridY) const;
		glm::vec2 GridToPixelCenter(int gridX, int gridY) const;
		glm::ivec2 PixelToGrid(float pixelX, float pixelY) const;

		int GetWidth() const { return m_width; }
		int GetHeight() const { return m_height; }
		int GetCellSize() const { return m_cellSize; }
		const glm::ivec2& GetGridOffset() const { return m_gridOffset; }

		int GetLayer(int gridY) const;

	private:
		int m_width;
		int m_height;
		int m_cellSize;
		glm::ivec2 m_gridOffset;

		std::vector<CellData> m_cells;

		std::shared_ptr<Texture2D> m_dirtTexture;
		std::shared_ptr<Texture2D> m_tunnelTexture;
		std::shared_ptr<Texture2D> m_surfaceTexture;

		int Index(int x, int y) const { return y * m_width + x; }
	};
}