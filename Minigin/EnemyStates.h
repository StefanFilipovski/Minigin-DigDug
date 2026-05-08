#pragma once
#include <glm/glm.hpp>

namespace dae
{
	class EnemyComponent;

	enum class EnemyStateType
	{
		Normal,
		Ghost,
		Inflating,
		Popped,
		Crushed,
		FireBreathing
	};

	// Base class State Pattern
	class EnemyState
	{
	public:
		virtual ~EnemyState() = default;

		virtual void Enter(EnemyComponent& enemy) = 0;
		virtual void Update(EnemyComponent& enemy, float deltaTime) = 0;
		virtual void Exit(EnemyComponent& enemy) = 0;
		virtual EnemyStateType GetType() const = 0;
	};

	// Concrete states

	class EnemyNormalState final : public EnemyState
	{
	public:
		void Enter(EnemyComponent& enemy) override;
		void Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::Normal; }

	private:
		float m_fireCooldown{ 0.f };
	};

	class EnemyGhostState final : public EnemyState
	{
	public:
		void Enter(EnemyComponent& enemy) override;
		void Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::Ghost; }

	private:
		bool m_hasBeenInDirt{ false };
	};

	class EnemyInflatingState final : public EnemyState
	{
	public:
		explicit EnemyInflatingState(const glm::ivec2& attackDir);

		void Enter(EnemyComponent& enemy) override;
		void Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::Inflating; }

		void PumpOnce(EnemyComponent& enemy);
		int GetInflateStage() const { return m_inflateStage; }

	private:
		glm::ivec2 m_attackDir;
		int m_inflateStage{ 1 };
		float m_deflateTimer{ 0.f };
	};

	class EnemyPoppedState final : public EnemyState
	{
	public:
		void Enter(EnemyComponent& enemy) override;
		void Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::Popped; }
	};

	class EnemyCrushedState final : public EnemyState
	{
	public:
		void Enter(EnemyComponent& enemy) override;
		void Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::Crushed; }
	};

	class EnemyFireBreathingState final : public EnemyState
	{
	public:
		void Enter(EnemyComponent& enemy) override;
		void Update(EnemyComponent& enemy, float deltaTime) override;
		void Exit(EnemyComponent& enemy) override;
		EnemyStateType GetType() const override { return EnemyStateType::FireBreathing; }

		const glm::ivec2& GetFireDirection() const { return m_fireDirection; }
		int GetCurrentFireRange() const { return m_currentRange; }

	private:
		glm::ivec2 m_fireDirection{ 1, 0 };
		float m_fireTimer{ 0.f };
		float m_fireDuration{ 1.2f };
		int m_maxFireRange{ 3 };
		int m_currentRange{ 0 };
		float m_extendTimer{ 0.f };
		float m_extendInterval{ 0.25f };
	};
}
