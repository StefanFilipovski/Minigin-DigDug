#include "InputManager.h"
#include <backends/imgui_impl_sdl3.h>
#include <cstring>

namespace dae
{
	bool InputManager::ProcessInput()
	{
		// Destroy commands from previous frame's state transitions
		m_commandGraveyard.clear();
		// Initialise controllers on first call
		if (m_Controllers.empty())
		{
			for (unsigned int i = 0; i < MaxControllers; ++i)
				m_Controllers.emplace_back(std::make_unique<Controller>(i));
		}

		// Poll controller state
		for (auto& controller : m_Controllers)
			controller->Update();

		// Get current keyboard state
		m_pKeyboardState = SDL_GetKeyboardState(nullptr);

		// SDL event loop
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			ImGui_ImplSDL3_ProcessEvent(&e);

			if (e.type == SDL_EVENT_QUIT)
				return false;
		}

		m_bindingsInvalidated = false;

		// Snapshot the keyboard bindings into a local vector of raw
		// (binding, command*) pairs. This way the loop iterates over
		// a stable snapshot and any clear/rebind during Execute() is safe.
		// The unique_ptrs still own the commands in the map — we just
		// borrow raw pointers for the duration of this frame.

		// --- Keyboard commands ---
		{
			// Build snapshot
			std::vector<std::pair<KeyboardBinding, Command*>> snapshot;
			snapshot.reserve(m_KeyboardCommands.size());
			for (const auto& [binding, pCmd] : m_KeyboardCommands)
				snapshot.emplace_back(binding, pCmd.get());

			for (const auto& [binding, pCmd] : snapshot)
			{
				// After a clear, the snapshot's pointers still point to alive
				// commands (moved to graveyard) but we stop because the map
				// now holds different bindings from the new state.
				if (m_bindingsInvalidated)
					break;

				const bool currentlyDown = m_pKeyboardState[binding.key] != 0;
				const bool previouslyDown = m_PreviousKeyState[binding.key] != 0;

				bool execute = false;
				switch (binding.state)
				{
				case KeyState::Down:    execute = currentlyDown && !previouslyDown;  break;
				case KeyState::Up:      execute = !currentlyDown && previouslyDown;  break;
				case KeyState::Pressed: execute = currentlyDown;                     break;
				}

				if (execute)
					pCmd->Execute();
			}
		}

		// --- Controller commands ---
		if (!m_bindingsInvalidated)
		{
			std::vector<std::pair<ControllerBinding, Command*>> snapshot;
			snapshot.reserve(m_ControllerCommands.size());
			for (const auto& [binding, pCmd] : m_ControllerCommands)
				snapshot.emplace_back(binding, pCmd.get());

			for (const auto& [binding, pCmd] : snapshot)
			{
				if (m_bindingsInvalidated)
					break;

				const auto& controller = m_Controllers[binding.controllerIdx];
				if (!controller->IsConnected()) continue;

				bool execute = false;
				switch (binding.state)
				{
				case KeyState::Down:    execute = controller->IsDownThisFrame(binding.button); break;
				case KeyState::Up:      execute = controller->IsUpThisFrame(binding.button);   break;
				case KeyState::Pressed: execute = controller->IsPressed(binding.button);       break;
				}

				if (execute)
					pCmd->Execute();
			}
		}

		// Save keyboard state for next frame
		std::memcpy(m_PreviousKeyState, m_pKeyboardState, SDL_SCANCODE_COUNT);

		return true;
	}

	void InputManager::BindKeyboardCommand(SDL_Scancode key, KeyState state,
		std::unique_ptr<Command> pCommand)
	{
		m_KeyboardCommands[{key, state}] = std::move(pCommand);
	}

	void InputManager::UnbindKeyboardCommand(SDL_Scancode key, KeyState state)
	{
		m_KeyboardCommands.erase({ key, state });
	}

	void InputManager::BindControllerCommand(unsigned int controllerIdx,
		Controller::Button button, KeyState state,
		std::unique_ptr<Command> pCommand)
	{
		m_ControllerCommands[{controllerIdx, button, state}] = std::move(pCommand);
	}

	void InputManager::UnbindControllerCommand(unsigned int controllerIdx,
		Controller::Button button, KeyState state)
	{
		m_ControllerCommands.erase({ controllerIdx, button, state });
	}

	void InputManager::ClearAllBindings()
	{
		// Move old commands into a graveyard — they stay alive until the
		// next call to ProcessInput so that any currently-executing command
		// (which lives in the old map) doesn't get destroyed mid-execution.
		for (auto& [key, cmd] : m_KeyboardCommands)
			m_commandGraveyard.push_back(std::move(cmd));
		m_KeyboardCommands.clear();

		for (auto& [key, cmd] : m_ControllerCommands)
			m_commandGraveyard.push_back(std::move(cmd));
		m_ControllerCommands.clear();

		m_bindingsInvalidated = true;
	}

	void InputManager::ClearKeyboardBindings()
	{
		for (auto& [key, cmd] : m_KeyboardCommands)
			m_commandGraveyard.push_back(std::move(cmd));
		m_KeyboardCommands.clear();
		m_bindingsInvalidated = true;
	}

	void InputManager::ClearControllerBindings()
	{
		for (auto& [key, cmd] : m_ControllerCommands)
			m_commandGraveyard.push_back(std::move(cmd));
		m_ControllerCommands.clear();
		m_bindingsInvalidated = true;
	}
}