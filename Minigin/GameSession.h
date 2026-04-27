#pragma once
#include "Singleton.h"
#include "GameMode.h"
#include <vector>
#include <string>
#include <fstream>

namespace dae
{
	// Holds persistent game session data that multiple states need access to.
	// This avoids states needing to know about each other's internals.
	class GameSession final : public Singleton<GameSession>
	{
	public:
		// Game mode
		void SetGameMode(GameMode mode) { m_gameMode = mode; }
		GameMode GetGameMode() const { return m_gameMode; }

		// Scores
		int GetPlayer1Score() const { return m_player1Score; }
		int GetPlayer2Score() const { return m_player2Score; }
		void SetPlayer1Score(int score) { m_player1Score = score; }
		void SetPlayer2Score(int score) { m_player2Score = score; }
		void AddPlayer1Score(int points) { m_player1Score += points; }
		void AddPlayer2Score(int points) { m_player2Score += points; }

		// Round tracking
		int GetCurrentRound() const { return m_currentRound; }
		void SetCurrentRound(int round) { m_currentRound = round; }

		// Lives
		int GetPlayer1Lives() const { return m_player1Lives; }
		int GetPlayer2Lives() const { return m_player2Lives; }
		void SetPlayer1Lives(int lives) { m_player1Lives = lives; }
		void SetPlayer2Lives(int lives) { m_player2Lives = lives; }

		// High scores
		struct HighScoreEntry
		{
			std::string name;  // 3 characters, arcade style
			int score;
		};

		const std::vector<HighScoreEntry>& GetHighScores() const { return m_highScores; }
		void AddHighScore(const std::string& name, int score);
		void LoadHighScores(const std::string& filepath);
		void SaveHighScores(const std::string& filepath) const;

		// Reset for new game
		void ResetForNewGame()
		{
			m_player1Score = 0;
			m_player2Score = 0;
			m_player1Lives = 4;  // 1 starting life + 3 extra
			m_player2Lives = 4;
			m_currentRound = 1;
		}

	private:
		friend class Singleton<GameSession>;
		GameSession() = default;

		GameMode m_gameMode{ GameMode::SinglePlayer };

		int m_player1Score{ 0 };
		int m_player2Score{ 0 };
		int m_player1Lives{ 4 };
		int m_player2Lives{ 4 };
		int m_currentRound{ 1 };

		std::vector<HighScoreEntry> m_highScores{};
		static constexpr size_t MaxHighScores{ 10 };
	};
}