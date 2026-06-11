#include "SpriteAnimatorComponent.h"
#include "RenderComponent.h"
#include "GameObject.h"

namespace dae
{
	// ---- AnimationSet ----

	void AnimationSet::Add(const std::string& name,
		const std::vector<SpriteFrame>& frames, float fps, bool loop)
	{
		SpriteAnimation anim;
		anim.name = name;
		anim.frames = frames;
		anim.fps = fps;
		anim.loop = loop;
		m_animations[name] = std::move(anim);
	}

	void AnimationSet::Add(const std::string& name, const std::string& texture,
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

	void AnimationSet::SetRenderSize(const std::string& name, float w, float h)
	{
		auto it = m_animations.find(name);
		if (it != m_animations.end())
		{
			it->second.renderW = w;
			it->second.renderH = h;
		}
	}

	const SpriteAnimation* AnimationSet::Find(const std::string& name) const
	{
		auto it = m_animations.find(name);
		return (it != m_animations.end()) ? &it->second : nullptr;
	}

	// ---- SpriteAnimatorComponent ----

	SpriteAnimatorComponent::SpriteAnimatorComponent(GameObject* pOwner)
		: Component(pOwner)
	{
	}

	void SpriteAnimatorComponent::SetAnimationSet(std::shared_ptr<const AnimationSet> set)
	{
		m_set = std::move(set);
		m_pCurrent = nullptr;
		m_currentAnimation.clear();
		m_currentFrame = 0;
		m_frameTimer = 0.f;
	}

	AnimationSet& SpriteAnimatorComponent::EnsureOwnedSet()
	{
		if (!m_ownedSet)
		{
			m_ownedSet = std::make_shared<AnimationSet>();
			m_set = m_ownedSet;
		}
		return *m_ownedSet;
	}

	void SpriteAnimatorComponent::AddAnimation(const std::string& name,
		const std::vector<SpriteFrame>& frames, float fps, bool loop)
	{
		EnsureOwnedSet().Add(name, frames, fps, loop);
	}

	void SpriteAnimatorComponent::AddAnimation(const std::string& name, const std::string& texture,
		const std::vector<SpriteFrame>& frames, float fps, bool loop)
	{
		EnsureOwnedSet().Add(name, texture, frames, fps, loop);
	}

	void SpriteAnimatorComponent::SetAnimationRenderSize(const std::string& name, float w, float h)
	{
		if (m_ownedSet)
			m_ownedSet->SetRenderSize(name, w, h);
	}

	void SpriteAnimatorComponent::Update(float deltaTime)
	{
		if (m_paused) return;
		if (!m_pCurrent || m_pCurrent->frames.empty()) return;

		m_frameTimer += deltaTime;
		float frameDuration = 1.f / m_pCurrent->fps;

		if (m_frameTimer >= frameDuration)
		{
			m_frameTimer -= frameDuration;
			++m_currentFrame;

			if (m_currentFrame >= static_cast<int>(m_pCurrent->frames.size()))
			{
				if (m_pCurrent->loop)
					m_currentFrame = 0;
				else
					m_currentFrame = static_cast<int>(m_pCurrent->frames.size()) - 1;
			}

			ApplyFrame();
		}
	}

	void SpriteAnimatorComponent::Play(const std::string& name)
	{
		if (m_currentAnimation == name)
			return;

		Restart(name);
	}

	void SpriteAnimatorComponent::Restart(const std::string& name)
	{
		if (!m_set) return;

		const SpriteAnimation* anim = m_set->Find(name);
		if (!anim) return;

		m_pCurrent = anim;
		m_currentAnimation = name;
		m_currentFrame = 0;
		m_frameTimer = 0.f;
		m_paused = false;

		ApplyFrame();
	}

	bool SpriteAnimatorComponent::IsAnimationFinished() const
	{
		if (!m_pCurrent) return false;
		if (m_pCurrent->loop) return false; // looping animations never "finish"
		return m_currentFrame >= static_cast<int>(m_pCurrent->frames.size()) - 1;
	}

	void SpriteAnimatorComponent::SetRenderSize(float w, float h)
	{
		m_renderWidth = w;
		m_renderHeight = h;
	}

	void SpriteAnimatorComponent::ApplyFrame()
	{
		if (!m_pRender)
		{
			m_pRender = GetOwner()->GetComponent<RenderComponent>();
			if (!m_pRender) return;
		}

		if (!m_pCurrent) return;
		if (m_currentFrame < 0 || m_currentFrame >= static_cast<int>(m_pCurrent->frames.size()))
			return;

		if (!m_pCurrent->texture.empty())
			m_pRender->SetTexture(m_pCurrent->texture);

		const auto& frame = m_pCurrent->frames[m_currentFrame];
		m_pRender->SetSourceRect(frame.x, frame.y, frame.w, frame.h);

		float rw = (m_pCurrent->renderW > 0.f) ? m_pCurrent->renderW : m_renderWidth;
		float rh = (m_pCurrent->renderH > 0.f) ? m_pCurrent->renderH : m_renderHeight;
		m_pRender->SetRenderSize(rw, rh);
	}
}
