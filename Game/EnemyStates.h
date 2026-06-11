#pragma once
#include <glm/glm.hpp>
#include <memory>

namespace dae
{
	class EnemyComponent;

	enum class EnemyStateType
	{
		Normal,
		Idle, // Player-controlled: no AI, just animations
		Ghost,
		Inflating,
		Popped,
		Crushed,
		FireBreathing
	};

	// Base class — states return a new state to transition, or nullptr to stay
	class EnemyState
	{
	public:
		virtual ~EnemyState() = default;

		virtual void Enter(EnemyComponent& enemy) = 0;
		virtual std::unique_ptr<EnemyState> Update(EnemyComponent& enemy, float deltaTime) = 0;
		virtual void Exit(EnemyComponent& enemy) = 0;
		virtual EnemyStateType GetType() const = 0;
	};

	// Player-controlled idle state — no AI, just updates walk animation based on movement
	class EnemyIdleState final : public EnemyState
	{
	public:
		void Enter(EnemyComponent& enemy) override;
		std::unique_ptr<EnemyState> Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::Idle; }
	};

	// Player-controlled ghost (Versus Fygar): phases through dirt, returning to
	// Idle once it reaches an empty cell (never surfaces inside the ground).
	class EnemyPlayerGhostState final : public EnemyState
	{
	public:
		void Enter(EnemyComponent& enemy) override;
		std::unique_ptr<EnemyState> Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::Ghost; }

	private:
		bool m_hasBeenInDirt{ false };
		float m_timer{ 0.f };
		float m_maxDuration{ 5.f }; // safety cap so the player can't ghost forever
	};

	// Normal tunnel movement — chases the player, decides ghost/fire transitions
	class EnemyNormalState final : public EnemyState
	{
	public:
		// ghostCooldown: pass remaining cooldown from FireBreathing to persist it
		explicit EnemyNormalState(float ghostCooldown = -1.f);

		void Enter(EnemyComponent& enemy) override;
		std::unique_ptr<EnemyState> Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::Normal; }

	private:
		glm::ivec2 ChooseDirection(EnemyComponent& enemy) const;
		bool ShouldBecomeGhost(EnemyComponent& enemy) const;

		float m_ghostCooldown{ -1.f };
		float m_fireCooldown{ 0.f };

		// Ghost transition settings
		float m_minGhostInterval{ 5.f };
		float m_maxGhostInterval{ 12.f };
	};

	// Phases through walls toward the player
	class EnemyGhostState final : public EnemyState
	{
	public:
		void Enter(EnemyComponent& enemy) override;
		std::unique_ptr<EnemyState> Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::Ghost; }

	private:
		glm::ivec2 ChooseGhostDirection(EnemyComponent& enemy) const;

		bool m_hasBeenInDirt{ false };
	};

	// Being pumped by the player
	class EnemyInflatingState final : public EnemyState
	{
	public:
		explicit EnemyInflatingState(const glm::ivec2& attackDir);

		void Enter(EnemyComponent& enemy) override;
		std::unique_ptr<EnemyState> Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::Inflating; }

		// Returns a new state (Popped) if max inflate reached, else nullptr
		std::unique_ptr<EnemyState> PumpOnce(EnemyComponent& enemy);
		int GetInflateStage() const { return m_inflateStage; }

	private:
		glm::ivec2 m_attackDir;
		int m_inflateStage{ 1 };
		float m_deflateTimer{ 0.f };
	};

	// Terminal state — enemy popped by pump
	class EnemyPoppedState final : public EnemyState
	{
	public:
		void Enter(EnemyComponent& enemy) override;
		std::unique_ptr<EnemyState> Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::Popped; }
	};

	// Terminal state — enemy crushed by rock
	class EnemyCrushedState final : public EnemyState
	{
	public:
		void Enter(EnemyComponent& enemy) override;
		std::unique_ptr<EnemyState> Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::Crushed; }
	};

	// Fygar fire attack
	class EnemyFireBreathingState final : public EnemyState
	{
	public:
		// Pass remaining ghost cooldown so it persists across this state
		explicit EnemyFireBreathingState(float ghostCooldown = -1.f);

		void Enter(EnemyComponent& enemy) override;
		std::unique_ptr<EnemyState> Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::FireBreathing; }

		const glm::ivec2& GetFireDirection() const { return m_fireDirection; }
		int GetCurrentFireRange() const { return m_currentRange; }
		float GetGhostCooldown() const { return m_ghostCooldown; }

	private:
		glm::ivec2 m_fireDirection{ 1, 0 };
		float m_fireTimer{ 0.f };
		float m_fireDuration{ 1.2f };
		int m_maxFireRange{ 3 };
		int m_currentRange{ 0 };
		float m_extendTimer{ 0.f };
		float m_extendInterval{ 0.25f };
		float m_ghostCooldown{ -1.f };
	};
}
