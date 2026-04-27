#if _DEBUG && __has_include(<vld.h>)
#include <vld.h>
#endif

#include <filesystem>
#include "Minigin.h"
#include "SceneManager.h"
#include "ResourceManager.h"

// Game state machine
#include "GameStateManager.h"
#include "GameSession.h"
#include "MenuState.h"
#include "PlayingState.h"
#include "GameOverState.h"
#include "HighScoreState.h"

namespace fs = std::filesystem;

static void load()
{
	// Initialize game session — load persistent high scores
	auto& session = dae::GameSession::GetInstance();
	session.LoadHighScores("Data/highscores.txt");

	// Register all game states
	auto& gsm = dae::GameStateManager::GetInstance();
	gsm.RegisterState<dae::MenuState>();
	gsm.RegisterState<dae::PlayingState>();
	gsm.RegisterState<dae::GameOverState>();
	gsm.RegisterState<dae::HighScoreState>();

	// Start at the menu
	gsm.SetState<dae::MenuState>();
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