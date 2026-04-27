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
		int x;  // Source x on spritesheet
		int y;  // Source y on spritesheet
		int w;  // Frame width (usually same for all frames)
		int h;  // Frame height
	};

	struct SpriteAnimation
	{
		std::string name;
		std::vector<SpriteFrame> frames;
		float fps{ 8.f };
		bool loop{ true };
	};

	class SpriteAnimatorComponent final : public Component
	{
	public:
		explicit SpriteAnimatorComponent(GameObject* pOwner);

		void Update(float deltaTime) override;

		// Add an animation
		void AddAnimation(const std::string& name, const std::vector<SpriteFrame>& frames,
			float fps = 8.f, bool loop = true);

		// Play an animation (no-op if already playing this one)
		void Play(const std::string& name);

		// Set the render size for frames (how big to draw on screen)
		void SetRenderSize(float w, float h);

		// Pause/resume
		void Pause() { m_paused = true; }
		void Resume() { m_paused = false; }
		bool IsPaused() const { return m_paused; }

		const std::string& GetCurrentAnimation() const { return m_currentAnimation; }
		int GetCurrentFrame() const { return m_currentFrame; }

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