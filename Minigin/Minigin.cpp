#include <stdexcept>
#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>

#if WIN32
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#endif

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Minigin.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "Renderer.h"
#include "ResourceManager.h"

using namespace std::chrono;

SDL_Window* g_window{};

void LogSDLVersion(const std::string& message, int major, int minor, int patch)
{
#if WIN32
	std::stringstream ss;
	ss << message << major << "." << minor << "." << patch << "\n";
	OutputDebugString(ss.str().c_str());
#else
	std::cout << message << major << "." << minor << "." << patch << "\n";
#endif
}

#ifdef __EMSCRIPTEN__
#include "emscripten.h"

void LoopCallback(void* arg)
{
	static_cast<dae::Minigin*>(arg)->RunOneFrame();
}
#endif

void PrintSDLVersion()
{
	LogSDLVersion("Compiled with SDL", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
	int version = SDL_GetVersion();
	LogSDLVersion("Linked with SDL ", SDL_VERSIONNUM_MAJOR(version), SDL_VERSIONNUM_MINOR(version), SDL_VERSIONNUM_MICRO(version));
	LogSDLVersion("Compiled with SDL_ttf ", SDL_TTF_MAJOR_VERSION, SDL_TTF_MINOR_VERSION, SDL_TTF_MICRO_VERSION);
	version = TTF_Version();
	LogSDLVersion("Linked with SDL_ttf ", SDL_VERSIONNUM_MAJOR(version), SDL_VERSIONNUM_MINOR(version), SDL_VERSIONNUM_MICRO(version));
}

dae::Minigin::Minigin(const std::filesystem::path& dataPath)
{
	PrintSDLVersion();

	if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
	{
		SDL_Log("Renderer error: %s", SDL_GetError());
		throw std::runtime_error(std::string("SDL_Init Error: ") + SDL_GetError());
	}

	g_window = SDL_CreateWindow(
		"Programming 4 assignment",
		1024,
		576,
		SDL_WINDOW_OPENGL
	);
	if (g_window == nullptr)
	{
		throw std::runtime_error(std::string("SDL_CreateWindow Error: ") + SDL_GetError());
	}

	Renderer::GetInstance().Init(g_window);
	ResourceManager::GetInstance().Init(dataPath);
}

dae::Minigin::~Minigin()
{
	Renderer::GetInstance().Destroy();
	SDL_DestroyWindow(g_window);
	g_window = nullptr;
	SDL_Quit();
}

void dae::Minigin::Run(const std::function<void()>& load)
{
	load();

	// Initialise the last time just before the loop starts
	m_lastTime = high_resolution_clock::now();

#ifndef __EMSCRIPTEN__
	while (!m_quit)
		RunOneFrame();
#else
	emscripten_set_main_loop_arg(&LoopCallback, this, 0, true);
#endif
}

void dae::Minigin::RunOneFrame()
{
	// Calculate delta time 
	const auto currentTime = high_resolution_clock::now();
	const float deltaTime = duration<float>(currentTime - m_lastTime).count();
	m_lastTime = currentTime;

	// Accumulate lag 
	m_lag += deltaTime;

	// Input 
	m_quit = !InputManager::GetInstance().ProcessInput();

	// Fixed update 
		while (m_lag >= FixedTimeStep)
		{
		SceneManager::GetInstance().FixedUpdate(FixedTimeStep);
		m_lag -= FixedTimeStep;
		}

	// Regular update
	SceneManager::GetInstance().Update(deltaTime);

	// Render 
	Renderer::GetInstance().Render();

	// Sleep
	const auto sleepTime = currentTime + std::chrono::duration<float, std::milli>(MsPerFrame) - high_resolution_clock::now();
	std::this_thread::sleep_for(sleepTime);
}