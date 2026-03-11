#include "InputManager.h"
#include <backends/imgui_impl_sdl3.h>
#include <cstring>

namespace dae
{
	bool InputManager::ProcessInput()
	{
		//Initialise controllers on first call
		if (m_Controllers.empty())
		{
			for (unsigned int i = 0; i < MaxControllers; ++i)
				m_Controllers.emplace_back(std::make_unique<Controller>(i));
		}

		//Poll controller state
		for (auto& controller : m_Controllers)
			controller->Update();

		// save previous state then get current
		m_pKeyboardState = SDL_GetKeyboardState(nullptr);
	

		//SDL event loop 
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			// ImGui
			ImGui_ImplSDL3_ProcessEvent(&e);

			if (e.type == SDL_EVENT_QUIT)
				return false;
		}

		// keyboard commands
		for (const auto& [binding, pCommand] : m_KeyboardCommands)
		{
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
				pCommand->Execute();
		}

		//  controller commands
		for (const auto& [binding, pCommand] : m_ControllerCommands)
		{
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
				pCommand->Execute();
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
}