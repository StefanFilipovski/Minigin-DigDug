#pragma once
#include "Component.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace dae
{
	class RenderComponent;

	struct SpriteFrame
	{
		int x;
		int y;
		int w;
		int h;
	};

	struct SpriteAnimation
	{
		std::string name;
		std::vector<SpriteFrame> frames;
		float fps{ 8.f };
		bool loop{ true };
		std::string texture;  
		float renderW{ 0.f };  
		float renderH{ 0.f };
	};

	
	class AnimationSet final
	{
	public:
		void Add(const std::string& name, const std::vector<SpriteFrame>& frames,
			float fps = 8.f, bool loop = true);
		void Add(const std::string& name, const std::string& texture,
			const std::vector<SpriteFrame>& frames, float fps = 8.f, bool loop = true);

	
		void SetRenderSize(const std::string& name, float w, float h);

		
		const SpriteAnimation* Find(const std::string& name) const;

	private:
		std::unordered_map<std::string, SpriteAnimation> m_animations;
	};

	class SpriteAnimatorComponent final : public Component
	{
	public:
		explicit SpriteAnimatorComponent(GameObject* pOwner);

		void Update(float deltaTime) override;

		
		void SetAnimationSet(std::shared_ptr<const AnimationSet> set);

		
		void AddAnimation(const std::string& name, const std::vector<SpriteFrame>& frames,
			float fps = 8.f, bool loop = true);
		void AddAnimation(const std::string& name, const std::string& texture,
			const std::vector<SpriteFrame>& frames, float fps = 8.f, bool loop = true);
		void SetAnimationRenderSize(const std::string& name, float w, float h);

		
		void Play(const std::string& name);

		
		void Restart(const std::string& name);

		void SetRenderSize(float w, float h);

		void Pause() { m_paused = true; }
		void Resume() { m_paused = false; }
		bool IsPaused() const { return m_paused; }

		const std::string& GetCurrentAnimation() const { return m_currentAnimation; }
		int GetCurrentFrame() const { return m_currentFrame; }
		bool IsAnimationFinished() const;

	private:
		void ApplyFrame();
		AnimationSet& EnsureOwnedSet();

		RenderComponent* m_pRender{ nullptr };

		std::shared_ptr<const AnimationSet> m_set; 
		std::shared_ptr<AnimationSet> m_ownedSet;  

		
		const SpriteAnimation* m_pCurrent{ nullptr };
		std::string m_currentAnimation;
		int m_currentFrame{ 0 };
		float m_frameTimer{ 0.f };
		bool m_paused{ false };

		float m_renderWidth{ 32.f };
		float m_renderHeight{ 32.f };
	};
}
