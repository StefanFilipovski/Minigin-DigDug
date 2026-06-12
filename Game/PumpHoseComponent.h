#pragma once
#include "Component.h"
#include <memory>
#include <glm/ext/vector_int2.hpp>

namespace dae
{
	class GridComponent;
	class Texture2D;

	
	class PumpHoseComponent final : public Component
	{
	public:
		PumpHoseComponent(GameObject* pOwner, GridComponent* pGrid);

		void Render() const override;

		// Driven by PumpComponent each frame
		void Show(const glm::ivec2& direction, float lengthCells, int rangeCells);
		void Hide() { m_visible = false; }

	private:
		GridComponent* m_pGrid;
		std::shared_ptr<Texture2D> m_hoseTexture;

		bool m_visible{ false };
		glm::ivec2 m_direction{ 1, 0 };
		float m_lengthCells{ 0.f };
		int m_rangeCells{ 0 };

		
		float m_hoseSrcW{ 32.f };
		float m_hoseSrcH{ 6.f };
		float m_hoseRenderLong{ 32.f };
		float m_hoseRenderShort{ 6.f };
	};
}
