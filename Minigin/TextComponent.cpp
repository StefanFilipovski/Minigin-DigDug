#include "TextComponent.h"
#include "TransformComponent.h"
#include "Renderer.h"
#include "Font.h"
#include "Texture2D.h"
#include "GameObject.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <stdexcept>

dae::TextComponent::TextComponent(GameObject* pOwner, std::shared_ptr<Font> font, const std::string& text)
	: Component(pOwner)
	, m_Font(std::move(font))
	, m_Text(text)
{
}

void dae::TextComponent::Update(float)
{
	if (m_NeedsUpdate)
	{
		RegenerateTexture();
		m_NeedsUpdate = false;
	}
}

void dae::TextComponent::Render() const
{
	if (!m_Texture) return;

	auto* transform = GetOwner()->GetComponent<TransformComponent>();
	if (!transform) return;

	const auto& pos = transform->GetPosition();
	Renderer::GetInstance().RenderTexture(*m_Texture, pos.x, pos.y);
}

void dae::TextComponent::SetText(const std::string& text)
{
	if (m_Text == text) return;
	m_Text = text;
	m_NeedsUpdate = true;
}

void dae::TextComponent::SetColor(SDL_Color color)
{
	m_Color = color;
	m_NeedsUpdate = true;
}

void dae::TextComponent::RegenerateTexture()
{
	SDL_Surface* surface = TTF_RenderText_Blended(m_Font->GetFont(), m_Text.c_str(), 0, m_Color);
	if (!surface)
		throw std::runtime_error("Failed to render text surface");

	SDL_Texture* sdlTexture = SDL_CreateTextureFromSurface(Renderer::GetInstance().GetSDLRenderer(), surface);
	SDL_DestroySurface(surface);

	if (!sdlTexture)
		throw std::runtime_error("Failed to create text texture");

	m_Texture = std::make_shared<Texture2D>(sdlTexture);
}