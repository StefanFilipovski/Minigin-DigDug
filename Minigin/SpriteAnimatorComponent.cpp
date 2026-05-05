#include "SpriteAnimatorComponent.h"
#include "RenderComponent.h"
#include "GameObject.h"

namespace dae
{
	SpriteAnimatorComponent::SpriteAnimatorComponent(GameObject* pOwner)
		: Component(pOwner)
	{
	}

	void SpriteAnimatorComponent::Update(float deltaTime)
	{
		if (m_paused) return;
		if (m_currentAnimation.empty()) return;

		auto it = m_animations.find(m_currentAnimation);
		if (it == m_animations.end()) return;

		const auto& anim = it->second;
		if (anim.frames.empty()) return;

		m_frameTimer += deltaTime;
		float frameDuration = 1.f / anim.fps;

		if (m_frameTimer >= frameDuration)
		{
			m_frameTimer -= frameDuration;
			++m_currentFrame;

			if (m_currentFrame >= static_cast<int>(anim.frames.size()))
			{
				if (anim.loop)
					m_currentFrame = 0;
				else
					m_currentFrame = static_cast<int>(anim.frames.size()) - 1;
			}

			ApplyFrame();
		}
	}

	void SpriteAnimatorComponent::AddAnimation(const std::string& name,
		const std::vector<SpriteFrame>& frames, float fps, bool loop)
	{
		SpriteAnimation anim;
		anim.name = name;
		anim.frames = frames;
		anim.fps = fps;
		anim.loop = loop;
		m_animations[name] = std::move(anim);
	}

	void SpriteAnimatorComponent::AddAnimation(const std::string& name, const std::string& texture,
		const std::vector<SpriteFrame>& frames, float fps, bool loop)
	{
		SpriteAnimation anim;
		anim.name = name;
		anim.texture = texture;
		anim.frames = frames;
		anim.fps = fps;
		anim.loop = loop;
		m_animations[name] = std::move(anim);
	}

	void SpriteAnimatorComponent::Play(const std::string& name)
	{
		if (m_currentAnimation == name)
			return;

		auto it = m_animations.find(name);
		if (it == m_animations.end())
			return;

		m_currentAnimation = name;
		m_currentFrame = 0;
		m_frameTimer = 0.f;
		m_paused = false;

		ApplyFrame();
	}

	void SpriteAnimatorComponent::SetRenderSize(float w, float h)
	{
		m_renderWidth = w;
		m_renderHeight = h;
	}

	void SpriteAnimatorComponent::SetAnimationRenderSize(const std::string& name, float w, float h)
	{
		auto it = m_animations.find(name);
		if (it != m_animations.end())
		{
			it->second.renderW = w;
			it->second.renderH = h;
		}
	}

	void SpriteAnimatorComponent::ApplyFrame()
	{
		if (!m_pRender)
		{
			m_pRender = GetOwner()->GetComponent<RenderComponent>();
			if (!m_pRender) return;
		}

		auto it = m_animations.find(m_currentAnimation);
		if (it == m_animations.end()) return;

		const auto& anim = it->second;
		if (m_currentFrame < 0 || m_currentFrame >= static_cast<int>(anim.frames.size()))
			return;

		if (!anim.texture.empty())
			m_pRender->SetTexture(anim.texture);

		const auto& frame = anim.frames[m_currentFrame];
		m_pRender->SetSourceRect(frame.x, frame.y, frame.w, frame.h);

		float rw = (anim.renderW > 0.f) ? anim.renderW : m_renderWidth;
		float rh = (anim.renderH > 0.f) ? anim.renderH : m_renderHeight;
		m_pRender->SetRenderSize(rw, rh);
	}
}