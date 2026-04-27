#pragma once
#include "GameState.h"
#include <string>
#include <array>

namespace dae
{
	class Scene;
	class GameObject;

	class HighScoreState final : public GameState
	{
	public:
		HighScoreState() = default;

		void OnEnter() override;
		void OnExit() override;
		void Update(float deltaTime) override;
		void FixedUpdate(float fixedTimeStep) override;
		void Render() const override;

		// Called by input commands
		void CycleCharacter(int direction);  // +1 = next letter, -1 = previous
		void ConfirmCharacter();             // Lock current letter, move to next
		void FinishEntry();                  // Submit the name

	private:
		void BindInput();
		void RebuildDisplay();

		Scene* m_pScene{ nullptr };

		// Arcade-style name entry: 3 characters, A-Z
		static constexpr int NameLength{ 3 };
		std::array<char, NameLength> m_nameChars{ 'A', 'A', 'A' };
		int m_currentCharIndex{ 0 };
		bool m_entryComplete{ false };

		// Display objects (raw pointers for updating text)
		GameObject* m_pNameDisplay{ nullptr };
		GameObject* m_pCursorDisplay{ nullptr };
		GameObject* m_pHighScoreList{ nullptr };

		static constexpr const char* HighScoreFilePath = "Data/highscores.txt";
	};
}