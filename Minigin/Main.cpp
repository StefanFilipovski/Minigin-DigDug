#if _DEBUG && __has_include(<vld.h>)
#include <vld.h>
#endif

#include <filesystem>
#include "Minigin.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "RenderComponent.h"
#include "BenchmarkComponent.h"
#include "InputManager.h"
#include "MoveCommand.h"
#include "TextComponent.h"
#include "FPSComponent.h"

namespace fs = std::filesystem;

static void load()
{
	auto& scene = dae::SceneManager::GetInstance().CreateScene();

	// Background
	auto background = std::make_unique<dae::GameObject>();
	background->AddComponent<dae::TransformComponent>();
	background->AddComponent<dae::RenderComponent>()->SetTexture("background.png");
	scene.Add(std::move(background));

	//Title logo
	auto logo = std::make_unique<dae::GameObject>();
	logo->AddComponent<dae::TransformComponent>()->SetLocalPosition(350.f, 216.f);
	logo->AddComponent<dae::RenderComponent>()->SetTexture("logo.png");
	scene.Add(std::move(logo));

	//fps counter
	auto fpsGo = std::make_unique<dae::GameObject>();
	fpsGo->AddComponent<dae::TransformComponent>()->SetLocalPosition(5.f, 20.f);
	auto font = dae::ResourceManager::GetInstance().LoadFont("Lingua.otf", 30);
	fpsGo->AddComponent<dae::TextComponent>(font, "FPS");
	fpsGo->AddComponent<dae::FPSComponent>();
	scene.Add(std::move(fpsGo));


	//Character 1: WASD keyboard, speed 100
	constexpr float speed1 = 2.f; // units per frame
	auto player1 = std::make_unique<dae::GameObject>();
	player1->AddComponent<dae::TransformComponent>()->SetLocalPosition(200.f, 400.f);
	player1->AddComponent<dae::RenderComponent>()->SetTexture("Batman.png");
	dae::GameObject* pPlayer1 = player1.get();
	scene.Add(std::move(player1));

	// Character 2: DPad controller (index 0), double speed
	constexpr float speed2 = speed1 * 2.f;
	auto player2 = std::make_unique<dae::GameObject>();
	player2->AddComponent<dae::TransformComponent>()->SetLocalPosition(400.f, 400.f);
	player2->AddComponent<dae::RenderComponent>()->SetTexture("Batman.png");
	dae::GameObject* pPlayer2 = player2.get();
	scene.Add(std::move(player2));

	//keyboard commands for player 1 (WASD)
	auto& input = dae::InputManager::GetInstance();

	input.BindKeyboardCommand(SDL_SCANCODE_W, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer1, glm::vec3{ 0,-1,0 }, speed1));
	input.BindKeyboardCommand(SDL_SCANCODE_S, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer1, glm::vec3{ 0, 1,0 }, speed1));
	input.BindKeyboardCommand(SDL_SCANCODE_A, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer1, glm::vec3{ -1,0,0 }, speed1));
	input.BindKeyboardCommand(SDL_SCANCODE_D, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer1, glm::vec3{ 1,0,0 }, speed1));

	// DPad commands for player 2 (controller 0) 
	input.BindControllerCommand(0, dae::Controller::Button::DPadUp, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer2, glm::vec3{ 0,-1,0 }, speed2));
	input.BindControllerCommand(0, dae::Controller::Button::DPadDown, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer2, glm::vec3{ 0, 1,0 }, speed2));
	input.BindControllerCommand(0, dae::Controller::Button::DPadLeft, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer2, glm::vec3{ -1,0,0 }, speed2));
	input.BindControllerCommand(0, dae::Controller::Button::DPadRight, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer2, glm::vec3{ 1,0,0 }, speed2));
		
}

int main(int, char* [])
{
#if __EMSCRIPTEN__
	fs::path data_location = "";
#else
	fs::path data_location = "./Data/";
	if (!fs::exists(data_location))
		data_location = "../Data/";
#endif
	dae::Minigin engine(data_location);
	engine.Run(load);
	return 0;
}