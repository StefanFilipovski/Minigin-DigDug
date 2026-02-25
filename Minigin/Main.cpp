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
#include "CircleMovementComponent.h"
#include "RotatorComponent.h"
#include "FPSComponent.h"
#include "TextComponent.h"

namespace fs = std::filesystem;

static void load()
{
	auto& scene = dae::SceneManager::GetInstance().CreateScene();

	// Background
	auto background = std::make_unique<dae::GameObject>();
	background->AddComponent<dae::TransformComponent>();
	background->AddComponent<dae::RenderComponent>()->SetTexture("background.png");
	scene.Add(std::move(background));


	// Logo
		auto logo = std::make_unique<dae::GameObject>();
	logo->AddComponent<dae::TransformComponent>()->SetLocalPosition(350.f, 216.f, 0.f);
	logo->AddComponent<dae::RenderComponent>()->SetTexture("logo.png");
	scene.Add(std::move(logo));

	// FPS counter
	auto fpsGo = std::make_unique<dae::GameObject>();
	auto* fpsTransform = fpsGo->AddComponent<dae::TransformComponent>();
	fpsTransform->SetPosition(10, 40);
	auto fpsFont = dae::ResourceManager::GetInstance().LoadFont("Lingua.otf", 50);
	fpsGo->AddComponent<dae::TextComponent>(fpsFont, "0 FPS");
	fpsGo->AddComponent<dae::FPSComponent>();
	scene.Add(std::move(fpsGo));

	// Character A: moves in a circle around the screen centre 
	auto charA = std::make_unique<dae::GameObject>();
	charA->AddComponent<dae::TransformComponent>();
	charA->AddComponent<dae::RenderComponent>()->SetTexture("Batman.png");
	charA->AddComponent<dae::CircleMovementComponent>(
		glm::vec3{ 300.f, 150.f, 0.f }, 
		150.f,
		1.0f                              
	);

	// Character B: orbits around Character A
	auto charB = std::make_unique<dae::GameObject>();
	charB->AddComponent<dae::TransformComponent>();
	charB->AddComponent<dae::RenderComponent>()->SetTexture("Batman.png");
	charB->AddComponent<dae::RotatorComponent>(
		60.f,    
		3.0f     
	);

	// Store raw pointers before moving ownership into the scene
	dae::GameObject* pCharA = charA.get();
	dae::GameObject* pCharB = charB.get();

	scene.Add(std::move(charA));
	scene.Add(std::move(charB));


	
	pCharB->SetParent(pCharA, false);
	
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