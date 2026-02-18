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

	private:
		void RegenerateTexture();

		std::string m_Text{};
		SDL_Color m_Color{ 255, 255, 255, 255 };
		std::shared_ptr<Font> m_Font{};
		std::shared_ptr<Texture2D> m_Texture{};
		bool m_NeedsUpdate{ true };
	};
}