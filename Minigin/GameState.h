#pragma once

namespace dae
{
	class GameState
	{
	public:
		virtual ~GameState() = default;

		virtual void OnEnter() = 0;
		virtual void OnExit() = 0;
		virtual void Update(float deltaTime) = 0;
		virtual void FixedUpdate(float fixedTimeStep) = 0;
		virtual void Render() const = 0;
		virtual void RenderImGui() {}

		GameState() = default;
		GameState(const GameState&) = delete;
		GameState(GameState&&) = delete;
		GameState& operator=(const GameState&) = delete;
		GameState& operator=(GameState&&) = delete;
	};
}