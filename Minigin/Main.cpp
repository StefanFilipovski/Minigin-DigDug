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
#include "TextComponent.h"
#include "FPSComponent.h"
#include "InputManager.h"
#include "MoveCommand.h"
#include "DieCommand.h"
#include "GainPointsCommand.h"
#include "HealthComponent.h"
#include "ScoreComponent.h"
#include "LivesDisplayComponent.h"
#include "PointsDisplayComponent.h"
#include "SteamAchievementObserver.h"

namespace fs = std::filesystem;

static dae::GameObject* MakePlayer(dae::Scene& scene, float x, float y)
{
	auto player = std::make_unique<dae::GameObject>();
	player->AddComponent<dae::TransformComponent>()->SetLocalPosition(x, y);
	player->AddComponent<dae::RenderComponent>()->SetTexture("Batman.png");
	player->AddComponent<dae::HealthComponent>(3);
	player->AddComponent<dae::ScoreComponent>();
	dae::GameObject* ptr = player.get();
	scene.Add(std::move(player));
	return ptr;
}

template<typename TDisplay, typename TSubject>
static void MakeDisplay(dae::Scene& scene, TSubject* pSubject,
	std::shared_ptr<dae::Font> font, float x, float y,
	const std::string& initialText)
{
	auto uiGo = std::make_unique<dae::GameObject>();
	uiGo->AddComponent<dae::TransformComponent>()->SetLocalPosition(x, y);
	uiGo->AddComponent<dae::TextComponent>(font, initialText);
	auto* pDisplay = uiGo->AddComponent<TDisplay>();
	pSubject->AddObserver(pDisplay);
	scene.Add(std::move(uiGo));
}

static void load()
{
	auto& scene = dae::SceneManager::GetInstance().CreateScene();
	auto& input = dae::InputManager::GetInstance();
	auto font = dae::ResourceManager::GetInstance().LoadFont("Lingua.otf", 18);

	// Background
	auto background = std::make_unique<dae::GameObject>();
	background->AddComponent<dae::TransformComponent>();
	background->AddComponent<dae::RenderComponent>()->SetTexture("background.png");
	scene.Add(std::move(background));

	// Title logo
	auto logo = std::make_unique<dae::GameObject>();
	logo->AddComponent<dae::TransformComponent>()->SetLocalPosition(216.f, 160.f);
	logo->AddComponent<dae::RenderComponent>()->SetTexture("logo.png");
	scene.Add(std::move(logo));

	// FPS
	auto fpsGo = std::make_unique<dae::GameObject>();
	fpsGo->AddComponent<dae::TransformComponent>()->SetLocalPosition(5.f, 5.f);
	fpsGo->AddComponent<dae::TextComponent>(font, "FPS");
	fpsGo->AddComponent<dae::FPSComponent>();
	scene.Add(std::move(fpsGo));

	// Controls hint
	auto hint = std::make_unique<dae::GameObject>();
	hint->AddComponent<dae::TransformComponent>()->SetLocalPosition(5.f, 540.f);
	hint->AddComponent<dae::TextComponent>(font,
		"P1: WASD=move  X=die  C=+100pts  |  P2: DPad=move  A=die  B=+100pts");
	scene.Add(std::move(hint));

	//Player 1: keyboard 
	constexpr float speed1 = 2.f;
	dae::GameObject* pPlayer1 = MakePlayer(scene, 200.f, 400.f);

	input.BindKeyboardCommand(SDL_SCANCODE_W, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer1, glm::vec3{ 0,-1,0 }, speed1));
	input.BindKeyboardCommand(SDL_SCANCODE_S, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer1, glm::vec3{ 0, 1,0 }, speed1));
	input.BindKeyboardCommand(SDL_SCANCODE_A, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer1, glm::vec3{ -1, 0,0 }, speed1));
	input.BindKeyboardCommand(SDL_SCANCODE_D, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer1, glm::vec3{ 1, 0,0 }, speed1));
	input.BindKeyboardCommand(SDL_SCANCODE_X, dae::KeyState::Down,
		std::make_unique<dae::DieCommand>(pPlayer1));
	input.BindKeyboardCommand(SDL_SCANCODE_C, dae::KeyState::Down,
		std::make_unique<dae::GainPointsCommand>(pPlayer1, 100));

	MakeDisplay<dae::LivesDisplayComponent>(scene,
		pPlayer1->GetComponent<dae::HealthComponent>(), font, 5.f, 30.f, "Lives: 3");
	MakeDisplay<dae::PointsDisplayComponent>(scene,
		pPlayer1->GetComponent<dae::ScoreComponent>(), font, 5.f, 50.f, "Score: 0");

	//Player 2: controller
	constexpr float speed2 = speed1 * 2.f;
	dae::GameObject* pPlayer2 = MakePlayer(scene, 400.f, 400.f);

	input.BindControllerCommand(0, dae::Controller::Button::DPadUp, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer2, glm::vec3{ 0,-1,0 }, speed2));
	input.BindControllerCommand(0, dae::Controller::Button::DPadDown, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer2, glm::vec3{ 0, 1,0 }, speed2));
	input.BindControllerCommand(0, dae::Controller::Button::DPadLeft, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer2, glm::vec3{ -1, 0,0 }, speed2));
	input.BindControllerCommand(0, dae::Controller::Button::DPadRight, dae::KeyState::Pressed,
		std::make_unique<dae::MoveCommand>(pPlayer2, glm::vec3{ 1, 0,0 }, speed2));
	input.BindControllerCommand(0, dae::Controller::Button::A, dae::KeyState::Down,
		std::make_unique<dae::DieCommand>(pPlayer2));
	input.BindControllerCommand(0, dae::Controller::Button::B, dae::KeyState::Down,
		std::make_unique<dae::GainPointsCommand>(pPlayer2, 100));

	MakeDisplay<dae::LivesDisplayComponent>(scene,
		pPlayer2->GetComponent<dae::HealthComponent>(), font, 5.f, 75.f, "Lives: 3");
	MakeDisplay<dae::PointsDisplayComponent>(scene,
		pPlayer2->GetComponent<dae::ScoreComponent>(), font, 5.f, 95.f, "Score: 0");

	//Steam achievement
	auto steamGo = std::make_unique<dae::GameObject>();
	auto* pSteamObserver = steamGo->AddComponent<dae::SteamAchievementObserver>();
	pPlayer1->GetComponent<dae::ScoreComponent>()->AddObserver(pSteamObserver);
	pPlayer2->GetComponent<dae::ScoreComponent>()->AddObserver(pSteamObserver);
	scene.Add(std::move(steamGo));
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