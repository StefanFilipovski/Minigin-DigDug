#pragma once
#include <SDL3/SDL.h>
#include "Singleton.h"

namespace dae
{
	class Texture2D;

	class Renderer final : public Singleton<Renderer>
	{
		SDL_Renderer* m_renderer{};
		SDL_Window* m_window{};
		SDL_Color m_clearColor{};
	public:
		void Init(SDL_Window* window);
		void Render() const;
		void Destroy();

		void RenderTexture(const Texture2D& texture, float x, float y) const;
		void RenderTexture(const Texture2D& texture, float x, float y, float width, float height) const;

		// Source rect overload for sprite sheets
		void RenderTexture(const Texture2D& texture, float x, float y,
			const SDL_FRect& srcRect, float dstWidth, float dstHeight) const;

		// Rotated draw — angle in degrees, clockwise. center is in destination space
		// (relative to dst.x/dst.y). Pass nullptr to rotate around the destination center.
		void RenderTextureRotated(const Texture2D& texture, float x, float y,
			float dstWidth, float dstHeight, double angleDegrees,
			const SDL_FPoint* center = nullptr) const;
		void RenderTextureRotated(const Texture2D& texture, float x, float y,
			const SDL_FRect& srcRect, float dstWidth, float dstHeight,
			double angleDegrees, const SDL_FPoint* center = nullptr) const;

		SDL_Renderer* GetSDLRenderer() const;

		const SDL_Color& GetBackgroundColor() const { return m_clearColor; }
		void SetBackgroundColor(const SDL_Color& color) { m_clearColor = color; }
	};
}