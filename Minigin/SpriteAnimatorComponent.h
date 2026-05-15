#pragma once
#include "Component.h"
#include <string>
#include <vector>
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
		std::string texture;   // empty = keep current texture
		float renderW{ 0.f };  // 0 = use animator's default render size
		float renderH{ 0.f };
	};

	class SpriteAnimatorComponent final : public Component
	{
	public:
		explicit SpriteAnimatorComponent(GameObject* pOwner);

		void Update(float deltaTime) override;

		void AddAnimation(const std::string& name, const std::vector<SpriteFrame>& frames,
			float fps = 8.f, bool loop = true);
		void AddAnimation(const std::string& name, const std::string& texture,
			const std::vector<SpriteFrame>& frames, float fps = 8.f, bool loop = true);

		// No-op if the requested animation is already playing
		void Play(const std::string& name);

		void SetRenderSize(float w, float h);
		void SetAnimationRenderSize(const std::string& name, float w, float h);

		void Pause() { m_paused = true; }
		void Resume() { m_paused = false; }
		bool IsPaused() const { return m_paused; }

		const std::string& GetCurrentAnimation() const { return m_currentAnimation; }
		int GetCurrentFrame() const { return m_currentFrame; }
		bool IsAnimationFinished() const;

	private:
		void ApplyFrame();

		RenderComponent* m_pRender{ nullptr };

		std::unordered_map<std::string, SpriteAnimation> m_animations;
		std::string m_currentAnimation;
		int m_currentFrame{ 0 };
		float m_frameTimer{ 0.f };
		bool m_paused{ false };

		float m_renderWidth{ 32.f };
		float m_renderHeight{ 32.f };
	};
}