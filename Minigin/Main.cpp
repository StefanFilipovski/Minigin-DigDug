#if _DEBUG && __has_include(<vld.h>)
#include <vld.h>
#endif

#if _DEBUG && __has_include(<vld.h>)
#include <vld.h>
#pragma message("VLD included")
#else
#pragma message("VLD NOT included")
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

namespace fs = std::filesystem;

static void load()
{
	auto& scene = dae::SceneManager::GetInstance().CreateScene();

	// Background
	auto background = std::make_unique<dae::GameObject>();
	background->AddComponent<dae::TransformComponent>();
	auto* bgRender = background->AddComponent<dae::RenderComponent>();
	bgRender->SetTexture("background.png");
	scene.Add(std::move(background));

	// Logo
	auto logo = std::make_unique<dae::GameObject>();
	auto* logoTransform = logo->AddComponent<dae::TransformComponent>();
	logoTransform->SetPosition(358, 180);
	auto* logoRender = logo->AddComponent<dae::RenderComponent>();
	logoRender->SetTexture("logo.png");
	scene.Add(std::move(logo));

	// Title text
	auto font = dae::ResourceManager::GetInstance().LoadFont("Lingua.otf", 36);
	auto titleGo = std::make_unique<dae::GameObject>();
	auto* titleTransform = titleGo->AddComponent<dae::TransformComponent>();
	titleTransform->SetPosition(292, 20);
	auto* titleText = titleGo->AddComponent<dae::TextComponent>(font, "Programming 4 Assignment");
	titleText->SetColor({ 255, 255, 0, 255 });
	scene.Add(std::move(titleGo));

	// FPS counter
	auto fpsGo = std::make_unique<dae::GameObject>();
	auto* fpsTransform = fpsGo->AddComponent<dae::TransformComponent>();
	fpsTransform->SetPosition(10, 40);
	auto fpsFont = dae::ResourceManager::GetInstance().LoadFont("Lingua.otf", 50);
	fpsGo->AddComponent<dae::TextComponent>(fpsFont, "0 FPS");
	fpsGo->AddComponent<dae::FPSComponent>();
	scene.Add(std::move(fpsGo));

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