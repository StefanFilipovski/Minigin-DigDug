#pragma once
#include <string>
#include <memory>
#include "Component.h"
#include <SDL3/SDL_pixels.h>

namespace dae
{
	class Font;
	class Texture2D;
	class TransformComponent;

	class TextComponent final : public Component
	{
	public:
		explicit TextComponent(GameObject* pOwner, std::shared_ptr<Font> font, const std::string& text = "");

		void Update(float deltaTime) override;
		void Render() const override;

		void SetText(const std::string& text);
		void SetColor(SDL_Color color);

		// Horizontally centre the text around centerX (the transform's X is then
		// ignored, only its Y is used). Re-centres automatically when the text
		// changes width. Pass the screen-centre X, e.g. 320 for a 640-wide window.
		void SetHorizontalCenter(float centerX) { m_Centered = true; m_CenterX = centerX; }

	private:
		void RegenerateTexture();

		std::string m_Text{};
		SDL_Color m_Color{ 255, 255, 255, 255 };
		std::shared_ptr<Font> m_Font{};
		std::shared_ptr<Texture2D> m_Texture{};
		bool m_NeedsUpdate{ true };
		bool m_Centered{ false };
		float m_CenterX{ 0.f };
	};
}