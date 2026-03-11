#include "Controller.h"

#if __EMSCRIPTEN__
// Emscripten/SDL fallback
#include <SDL3/SDL.h>

namespace dae
{
	class Controller::ControllerImpl final
	{
	public:
		explicit ControllerImpl(unsigned int index)
			: m_ControllerIndex(index)
		{
		}

		void Update()
		{
			
			if (!m_pGamepad)
			{
				int count{};
				SDL_JoystickID* ids = SDL_GetGamepads(&count);
				if (ids && m_ControllerIndex < static_cast<unsigned>(count))
				{
					m_pGamepad = SDL_OpenGamepad(ids[m_ControllerIndex]);
				}
				SDL_free(ids);
			}

			m_ButtonsPressedThisFrame = 0;
			m_ButtonsReleasedThisFrame = 0;

			if (!m_pGamepad) return;

			unsigned int current = 0;
			
			auto check = [&](SDL_GamepadButton sdlBtn, unsigned int bit)
				{
					if (SDL_GetGamepadButton(m_pGamepad, sdlBtn))
						current |= bit;
				};
			check(SDL_GAMEPAD_BUTTON_DPAD_UP, 0x0001);
			check(SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0x0002);
			check(SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0x0004);
			check(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0x0008);
			check(SDL_GAMEPAD_BUTTON_START, 0x0010);
			check(SDL_GAMEPAD_BUTTON_BACK, 0x0020);
			check(SDL_GAMEPAD_BUTTON_LEFT_STICK, 0x0040);
			check(SDL_GAMEPAD_BUTTON_RIGHT_STICK, 0x0080);
			check(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 0x0100);
			check(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0x0200);
			check(SDL_GAMEPAD_BUTTON_SOUTH, 0x1000); 
			check(SDL_GAMEPAD_BUTTON_EAST, 0x2000); 
			check(SDL_GAMEPAD_BUTTON_WEST, 0x4000); 
			check(SDL_GAMEPAD_BUTTON_NORTH, 0x8000); 

			unsigned int changes = current ^ m_ButtonsPrevious;
			m_ButtonsPressedThisFrame = changes & current;
			m_ButtonsReleasedThisFrame = changes & ~current;
			m_ButtonsCurrent = current;
			m_ButtonsPrevious = current;
		}

		bool IsDownThisFrame(unsigned int button) const { return (m_ButtonsPressedThisFrame & button) != 0; }
		bool IsUpThisFrame(unsigned int button) const { return (m_ButtonsReleasedThisFrame & button) != 0; }
		bool IsPressed(unsigned int button) const { return (m_ButtonsCurrent & button) != 0; }
		bool IsConnected()                    const { return m_pGamepad != nullptr; }

		~ControllerImpl()
		{
			if (m_pGamepad)
				SDL_CloseGamepad(m_pGamepad);
		}

	private:
		unsigned int   m_ControllerIndex;
		SDL_Gamepad* m_pGamepad{ nullptr };
		unsigned int   m_ButtonsCurrent{ 0 };
		unsigned int   m_ButtonsPrevious{ 0 };
		unsigned int   m_ButtonsPressedThisFrame{ 0 };
		unsigned int   m_ButtonsReleasedThisFrame{ 0 };
	};
}

#else
// Windows and XInput
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <XInput.h>
#pragma comment(lib, "xinput.lib")

namespace dae
{
	class Controller::ControllerImpl final
	{
	public:
		explicit ControllerImpl(unsigned int controllerIndex)
			: m_ControllerIndex(controllerIndex)
		{
			ZeroMemory(&m_CurrentState, sizeof(XINPUT_STATE));
			ZeroMemory(&m_PreviousState, sizeof(XINPUT_STATE));
		}

		void Update()
		{
			CopyMemory(&m_PreviousState, &m_CurrentState, sizeof(XINPUT_STATE));
			ZeroMemory(&m_CurrentState, sizeof(XINPUT_STATE));
			m_IsConnected = (XInputGetState(m_ControllerIndex, &m_CurrentState) == ERROR_SUCCESS);

			if (!m_IsConnected) return;

			const WORD buttonChanges =
				m_CurrentState.Gamepad.wButtons ^ m_PreviousState.Gamepad.wButtons;

			m_ButtonsPressedThisFrame = buttonChanges & m_CurrentState.Gamepad.wButtons;
			m_ButtonsReleasedThisFrame = buttonChanges & (~m_CurrentState.Gamepad.wButtons);
		}

		bool IsDownThisFrame(unsigned int button) const
		{
			return (m_ButtonsPressedThisFrame & button) != 0;
		}
		bool IsUpThisFrame(unsigned int button) const
		{
			return (m_ButtonsReleasedThisFrame & button) != 0;
		}
		bool IsPressed(unsigned int button) const
		{
			return (m_CurrentState.Gamepad.wButtons & button) != 0;
		}
		bool IsConnected() const { return m_IsConnected; }

	private:
		unsigned int  m_ControllerIndex;
		XINPUT_STATE  m_CurrentState{};
		XINPUT_STATE  m_PreviousState{};
		WORD          m_ButtonsPressedThisFrame{ 0 };
		WORD          m_ButtonsReleasedThisFrame{ 0 };
		bool          m_IsConnected{ false };
	};
}
#endif

//Controller that delegates to pimpl
namespace dae
{
	Controller::Controller(unsigned int controllerIndex)
		: m_pImpl(std::make_unique<ControllerImpl>(controllerIndex))
	{
	}

	Controller::~Controller() = default;

	void Controller::Update() { m_pImpl->Update(); }
	bool Controller::IsConnected()   const { return m_pImpl->IsConnected(); }

	bool Controller::IsDownThisFrame(Button button) const
	{
		return m_pImpl->IsDownThisFrame(static_cast<unsigned int>(button));
	}
	bool Controller::IsUpThisFrame(Button button) const
	{
		return m_pImpl->IsUpThisFrame(static_cast<unsigned int>(button));
	}
	bool Controller::IsPressed(Button button) const
	{
		return m_pImpl->IsPressed(static_cast<unsigned int>(button));
	}
}