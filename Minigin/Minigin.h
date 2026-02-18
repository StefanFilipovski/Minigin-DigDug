#pragma once
#include <string>
#include <functional>
#include <filesystem>
#include <chrono>

namespace dae
{
	class Minigin final
	{
	public:
		explicit Minigin(const std::filesystem::path& dataPath);
		~Minigin();
		void Run(const std::function<void()>& load);
		void RunOneFrame();

		Minigin(const Minigin& other) = delete;
		Minigin(Minigin&& other) = delete;
		Minigin& operator=(const Minigin& other) = delete;
		Minigin& operator=(Minigin&& other) = delete;

	private:
		bool m_quit{};

		// Timing
		std::chrono::high_resolution_clock::time_point m_lastTime{};
		float m_lag{};

		static constexpr float MsPerFrame{ 1000.f / 60.f }; // 60 FPS cap
		static constexpr float	FixedTimeStep{ 0.02f };	 // 50Hz fixed update
	};
}