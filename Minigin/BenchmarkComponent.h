#pragma once
#include "Component.h"
#include <vector>
#include <string>
#include <functional>

namespace dae
{
	class BenchmarkComponent final : public Component
	{
	public:
		explicit BenchmarkComponent(GameObject* pOwner);

		void RenderImGui() override;

	private:
		// Runs a single pass over the integer array at given step, returns microseconds
		static long long RunIntPass(int* arr, int size, int step);

		// Runs a single pass over the GameObject3D array at given step, returns microseconds
		struct GameObject3D;
		static long long RunGO3DPass(GameObject3D* arr, int size, int step);

		// Runs a single pass over the Alt array (only IDs) at given step, returns microseconds
		static long long RunGO3DAltPass(int* ids, int size, int step);

		// multiple passes, trim outliers, average
		static float RunBenchmark(int step, int runs,
			std::function<long long(int)> singlePass);

		static constexpr int  BufferSize{ 1 << 26 }; 
		static constexpr int  MaxStep{ 1024 };
		static constexpr int  Runs{ 10 };       
		static constexpr int  TrimCount{ 2 };       

		std::vector<std::string> m_stepLabels{};

		// Results
		std::vector<float> m_intResults{};
		std::vector<float> m_go3dResults{};
		std::vector<float> m_go3dAltResults{};
	};
}