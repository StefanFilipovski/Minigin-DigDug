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
		std::string texture;   // empty = keep current texture
		float renderW{ 0.f };  // 0 = use animator's default render size
		float renderH{ 0.f };
	};

	// Flyweight: the intrinsic animation data (frames, fps, textures) that is
	// identical for every instance of a character type. Build one set per type
	// and share it between animators via shared_ptr — each animator then only
	// stores its own playback state (current animation, frame, timer).
	class AnimationSet final
	{
	public:
		void Add(const std::string& name, const std::vector<SpriteFrame>& frames,
			float fps = 8.f, bool loop = true);
		void Add(const std::string& name, const std::string& texture,
			const std::vector<SpriteFrame>& frames, float fps = 8.f, bool loop = true);

		// Per-animation render size override (e.g. smaller ghost sprite).
		// Call during set construction only — sets are immutable once shared.
		void SetRenderSize(const std::string& name, float w, float h);

		// Returns nullptr if no animation with that name exists. The pointer
		// stays valid for the lifetime of the set (we never erase entries).
		const SpriteAnimation* Find(const std::string& name) const;

	private:
		std::unordered_map<std::string, SpriteAnimation> m_animations;
	};

	class SpriteAnimatorComponent final : public Component
	{
	public:
		explicit SpriteAnimatorComponent(GameObject* pOwner);

		void Update(float deltaTime) override;

		// Share an immutable, prebuilt animation set (flyweight). Preferred
		// when many objects of the same type use identical animations.
		void SetAnimationSet(std::shared_ptr<const AnimationSet> set);

		// Convenience for one-off animators: builds a set owned by this
		// instance. Don't mix with SetAnimationSet.
		void AddAnimation(const std::string& name, const std::vector<SpriteFrame>& frames,
			float fps = 8.f, bool loop = true);
		void AddAnimation(const std::string& name, const std::string& texture,
			const std::vector<SpriteFrame>& frames, float fps = 8.f, bool loop = true);
		void SetAnimationRenderSize(const std::string& name, float w, float h);

		// No-op if the requested animation is already playing
		void Play(const std::string& name);

		// Like Play, but always restarts from frame 0 and re-applies the source
		// rect even if the animation is already current. Use this after the
		// render state may have been cleared (e.g. respawn after a death).
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

		std::shared_ptr<const AnimationSet> m_set;  // shared flyweight (or owned set)
		std::shared_ptr<AnimationSet> m_ownedSet;   // only when built via AddAnimation

		// Extrinsic per-instance playback state
		const SpriteAnimation* m_pCurrent{ nullptr }; // cached — no map lookup per frame
		std::string m_currentAnimation;
		int m_currentFrame{ 0 };
		float m_frameTimer{ 0.f };
		bool m_paused{ false };

		float m_renderWidth{ 32.f };
		float m_renderHeight{ 32.f };
	};
}
