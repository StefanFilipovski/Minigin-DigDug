#include "RenderComponent.h"
#include "TransformComponent.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "Texture2D.h"
#include "GameObject.h"

namespace dae
{
	void RenderComponent::Render() const
	{
		if (!m_Texture) return;

		auto* transform = GetOwner()->GetComponent<TransformComponent>();
		if (!transform) return;

		const auto& pos = transform->GetWorldPosition();

		if (m_flipH)
		{
			// Use SDL_RenderTextureRotated with FLIP_HORIZONTAL
			auto* sdlRenderer = Renderer::GetInstance().GetSDLRenderer();
			auto* sdlTexture = m_Texture->GetSDLTexture();

			float dstW{}, dstH{};
			const SDL_FRect* pSrc = nullptr;

			if (m_useSourceRect)
			{
				pSrc = &m_sourceRect;
				dstW = m_useRenderSize ? m_renderWidth : m_sourceRect.w;
				dstH = m_useRenderSize ? m_renderHeight : m_sourceRect.h;
			}
			else if (m_useRenderSize)
			{
				dstW = m_renderWidth;
				dstH = m_renderHeight;
			}
			else
			{
				SDL_GetTextureSize(sdlTexture, &dstW, &dstH);
			}

			SDL_FRect dst{ pos.x, pos.y, dstW, dstH };
			SDL_RenderTextureRotated(sdlRenderer, sdlTexture,
				pSrc, &dst, 0.0, nullptr, SDL_FLIP_HORIZONTAL);
		}
		else if (m_useSourceRect)
		{
			Renderer::GetInstance().RenderTexture(*m_Texture, pos.x, pos.y,
				m_sourceRect,
				m_useRenderSize ? m_renderWidth : m_sourceRect.w,
				m_useRenderSize ? m_renderHeight : m_sourceRect.h);
		}
		else if (m_useRenderSize)
		{
			Renderer::GetInstance().RenderTexture(*m_Texture, pos.x, pos.y,
				m_renderWidth, m_renderHeight);
		}
		else
		{
			Renderer::GetInstance().RenderTexture(*m_Texture, pos.x, pos.y);
		}
	}

	void RenderComponent::SetTexture(const std::string& filename)
	{
		m_Texture = ResourceManager::GetInstance().LoadTexture(filename);
	}

	void RenderComponent::SetTexture(std::shared_ptr<Texture2D> texture)
	{
		m_Texture = texture;
	}

	void RenderComponent::SetSourceRect(int x, int y, int w, int h)
	{
		m_sourceRect.x = static_cast<float>(x);
		m_sourceRect.y = static_cast<float>(y);
		m_sourceRect.w = static_cast<float>(w);
		m_sourceRect.h = static_cast<float>(h);
		m_useSourceRect = true;
	}

	void RenderComponent::ClearSourceRect()
	{
		m_useSourceRect = false;
	}

	void RenderComponent::SetRenderSize(float w, float h)
	{
		m_renderWidth = w;
		m_renderHeight = h;
		m_useRenderSize = true;
	}

	void RenderComponent::ClearRenderSize()
	{
		m_useRenderSize = false;
	}
}