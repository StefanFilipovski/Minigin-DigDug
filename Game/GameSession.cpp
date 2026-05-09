#include "GameSession.h"
#include <algorithm>
#include <iostream>

namespace dae
{
	void GameSession::AddHighScore(const std::string& name, int score)
	{
		m_highScores.push_back({ name, score });

		// Sort descending by score
		std::sort(m_highScores.begin(), m_highScores.end(),
			[](const HighScoreEntry& a, const HighScoreEntry& b)
			{
				return a.score > b.score;
			});

		// Keep only top N
		if (m_highScores.size() > MaxHighScores)
			m_highScores.resize(MaxHighScores);
	}

	void GameSession::LoadHighScores(const std::string& filepath)
	{
		m_highScores.clear();

		std::ifstream file(filepath);
		if (!file.is_open()) return;  // No high scores yet, that's fine

		std::string name;
		int score;
		while (file >> name >> score)
		{
			m_highScores.push_back({ name, score });
		}

		// Ensure sorted
		std::sort(m_highScores.begin(), m_highScores.end(),
			[](const HighScoreEntry& a, const HighScoreEntry& b)
			{
				return a.score > b.score;
			});

		if (m_highScores.size() > MaxHighScores)
			m_highScores.resize(MaxHighScores);
	}

	void GameSession::SaveHighScores(const std::string& filepath) const
	{
		std::ofstream file(filepath);
		if (!file.is_open())
		{
			std::cerr << "[GameSession] Could not save high scores to: " << filepath << "\n";
			return;
		}

		for (const auto& entry : m_highScores)
		{
			file << entry.name << " " << entry.score << "\n";
		}
	}
}