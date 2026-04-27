#pragma once
#include "Singleton.h"
#include "GameState.h"
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <cassert>

namespace dae
{
	class GameStateManager final : public Singleton<GameStateManager>
	{
	public:
		// Register a state so it can be switched to later by type.
		// Returns a raw pointer for optional further setup.
		template <typename T, typename... Args>
		T* RegisterState(Args&&... args)
		{
			static_assert(std::is_base_of_v<GameState, T>, "T must derive from GameState");
			auto state = std::make_unique<T>(std::forward<Args>(args)...);
			T* ptr = state.get();
			m_States[typeid(T)] = std::move(state);
			return ptr;
		}

		// Transition to a registered state by type
		template <typename T>
		void SetState()
		{
			auto it = m_States.find(typeid(T));
			assert(it != m_States.end() && "State not registered! Call RegisterState<T>() first.");

			if (m_pCurrentState)
				m_pCurrentState->OnExit();

			m_pCurrentState = it->second.get();

			// Note: We do NOT clear input bindings here because SetState can be
			// called from inside a command's Execute (during ProcessInput iteration).
			// Clearing would destroy the command that's currently executing — UB.
			// Instead, each state's OnEnter() calls ClearAllBindings() first,
			// then adds its own bindings. The clear sets m_bindingsInvalidated
			// which makes ProcessInput break out of its snapshot loop safely.

			m_pCurrentState->OnEnter();
		}

		void Update(float deltaTime)
		{
			if (m_pCurrentState)
				m_pCurrentState->Update(deltaTime);
		}

		void FixedUpdate(float fixedTimeStep)
		{
			if (m_pCurrentState)
				m_pCurrentState->FixedUpdate(fixedTimeStep);
		}

		void Render() const
		{
			if (m_pCurrentState)
				m_pCurrentState->Render();
		}

		void RenderImGui()
		{
			if (m_pCurrentState)
				m_pCurrentState->RenderImGui();
		}

		GameState* GetCurrentState() const { return m_pCurrentState; }

	private:
		friend class Singleton<GameStateManager>;
		GameStateManager() = default;

		std::unordered_map<std::type_index, std::unique_ptr<GameState>> m_States{};
		GameState* m_pCurrentState{ nullptr };
	};
}