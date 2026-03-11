#pragma once
#include "Singleton.h"
#include "Command.h"
#include "Controller.h"
#include <SDL3/SDL.h>
#include <memory>
#include <vector>
#include <map>

namespace dae
{
	enum class KeyState
	{
		Down,      
		Up,        
		Pressed,   
	};

	class InputManager final : public Singleton<InputManager>
	{
	public:
		bool ProcessInput();

		//Keyboard binding 
		void BindKeyboardCommand(SDL_Scancode key, KeyState state, std::unique_ptr<Command> pCommand);
		void UnbindKeyboardCommand(SDL_Scancode key, KeyState state);

		//Controller binding 
		void BindControllerCommand(unsigned int controllerIdx, Controller::Button button,
			KeyState state, std::unique_ptr<Command> pCommand);
		void UnbindControllerCommand(unsigned int controllerIdx, Controller::Button button,
			KeyState state);

	private:
		friend class Singleton<InputManager>;
		InputManager() = default;

		//Keyboard state tracking
		const bool* m_pKeyboardState{ nullptr };
		bool m_PreviousKeyState[SDL_SCANCODE_COUNT]{};

		// Controllers
		static constexpr unsigned int MaxControllers{ 4 };
		std::vector<std::unique_ptr<Controller>> m_Controllers{};

		//Binding maps
		struct KeyboardBinding
		{
			SDL_Scancode key;
			KeyState     state;
		};
		struct ControllerBinding
		{
			unsigned int      controllerIdx;
			Controller::Button button;
			KeyState           state;
		};

		struct KeyboardBindingCmp
		{
			bool operator()(const KeyboardBinding& a, const KeyboardBinding& b) const
			{
				if (a.key != b.key) return a.key < b.key;
				return a.state < b.state;
			}
		};
		struct ControllerBindingCmp
		{
			bool operator()(const ControllerBinding& a, const ControllerBinding& b) const
			{
				if (a.controllerIdx != b.controllerIdx) return a.controllerIdx < b.controllerIdx;
				if (a.button != b.button) return a.button < b.button;
				return a.state < b.state;
			}
		};

		std::map<KeyboardBinding, std::unique_ptr<Command>, KeyboardBindingCmp>   m_KeyboardCommands{};
		std::map<ControllerBinding, std::unique_ptr<Command>, ControllerBindingCmp> m_ControllerCommands{};
	};
}