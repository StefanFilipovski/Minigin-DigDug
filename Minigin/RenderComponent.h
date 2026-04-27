#pragma once
#include <memory>
#include <string>
#include <SDL3/SDL.h>
#include "Component.h"

namespace dae
{
	class Texture2D;
	class TransformComponent;

	class RenderComponent final : public Component
	{
	public:
		explicit RenderComponent(GameObject* pOwner) : Component(pOwner) {}

		void Render() const override;
		void SetTexture(const std::string& filename);
		void SetTexture(std::shared_ptr<Texture2D> texture);

		// Source rectangle for sprite sheets — defines which part of the texture to draw
		void SetSourceRect(int x, int y, int w, int h);
		void ClearSourceRect();
		bool HasSourceRect() const { return m_useSourceRect; }

		// Destination size override — draws at this size instead of texture size
		void SetRenderSize(float w, float h);
		void ClearRenderSize();

	private:
		std::shared_ptr<Texture2D> m_Texture{};

		SDL_FRect m_sourceRect{};
		bool m_useSourceRect{ false };

		float m_renderWidth{ 0.f };
		float m_renderHeight{ 0.f };
		bool m_useRenderSize{ false };
	};
}