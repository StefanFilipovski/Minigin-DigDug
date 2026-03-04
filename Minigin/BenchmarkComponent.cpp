#include "BenchmarkComponent.h"
#include <iostream>
#include <iomanip>
#include <imgui.h>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <functional>
#include <memory>
#include <string>

namespace dae
{
	
	
	//Data structures for Exercise 2
	struct Transform
	{
		float matrix[16] = {
			1,0,0,0,
			0,1,0,0,
			0,0,1,0,
			0,0,0,1 };
	};

	// Origina ID and Transform together in memory.
	struct BenchmarkComponent::GameObject3D
	{
		Transform transform;
		int ID;
	};

	// Alternative when IDs are stored separately from Transforms.
	
	// Helpers
	long long BenchmarkComponent::RunIntPass(int* arr, int size, int step)
	{
		const auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < size; i += step)
			arr[i] *= 2;
		const auto end = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	}

	long long BenchmarkComponent::RunGO3DPass(GameObject3D* arr, int size, int step)
	{
		const auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < size; i += step)
			arr[i].ID *= 2;
		const auto end = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	}

	long long BenchmarkComponent::RunGO3DAltPass(int* ids, int size, int step)
	{
		const auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < size; i += step)
			ids[i] *= 2;
		const auto end = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	}

	// Run runs passes, sort, trim TrimCount from each end, average the rest
	float BenchmarkComponent::RunBenchmark(int step, int runs,
		std::function<long long(int)> singlePass)
	{
		std::vector<long long> timings(runs);
		for (int r = 0; r < runs; ++r)
			timings[r] = singlePass(step);

		std::sort(timings.begin(), timings.end());
		timings.erase(timings.begin(), timings.begin() + TrimCount);
		timings.erase(timings.end() - TrimCount, timings.end());

		const double avg = std::accumulate(timings.begin(), timings.end(), 0LL)
			/ static_cast<double>(timings.size());
		return static_cast<float>(avg);
	}

	
	// Constructor — run all benchmarks once on startup
	BenchmarkComponent::BenchmarkComponent(GameObject* pOwner)
		: Component(pOwner)
	{
		// Exercise 1: integer array 
		auto intBuf = std::make_unique<int[]>(BufferSize);
		for (int i = 0; i < BufferSize; ++i)
			intBuf[i] = i;

		// Exercise 2: GameObject3D array 
		constexpr int GO3DSize = BufferSize / 4;
		auto go3dBuf = std::make_unique<GameObject3D[]>(GO3DSize);
		for (int i = 0; i < GO3DSize; ++i)
			go3dBuf[i].ID = i;

		// Exercise 2: separate ID array 
		auto altIds = std::make_unique<int[]>(GO3DSize);
		for (int i = 0; i < GO3DSize; ++i)
			altIds[i] = i;

		std::cout << "\n=== Running Benchmarks ===\n";
		std::cout << "Buffer size: " << BufferSize << " ints, " << GO3DSize << " GameObjects\n";
		std::cout << "Runs per step: " << Runs << " (trimming " << TrimCount << " outliers each side)\n\n";

		//Run benchmarks for each step 
		std::cout << std::left
			<< std::setw(8) << "Step"
			<< std::setw(16) << "Int (us)"
			<< std::setw(20) << "GO3D AoS (us)"
			<< std::setw(20) << "GO3D SoA (us)" << "\n";
		std::cout << std::string(64, '-') << "\n";

		for (int step = 1; step <= MaxStep; step *= 2)
		{
			m_stepLabels.push_back(std::to_string(step));

			// Exercise 1
			const float intResult = RunBenchmark(step, Runs,
				[&](int s) { return RunIntPass(intBuf.get(), BufferSize, s); });
			m_intResults.push_back(intResult);

			// Exercise 2 original GameObject3D 
			const float go3dResult = RunBenchmark(step, Runs,
				[&](int s) { return RunGO3DPass(go3dBuf.get(), GO3DSize, s); });
			m_go3dResults.push_back(go3dResult);

			// Exercise 2 alt 
			const float altResult = RunBenchmark(step, Runs,
				[&](int s) { return RunGO3DAltPass(altIds.get(), GO3DSize, s); });
			m_go3dAltResults.push_back(altResult);

			// Print progress to console
			std::cout << std::left
				<< std::setw(8) << step
				<< std::setw(16) << intResult
				<< std::setw(20) << go3dResult
				<< std::setw(20) << altResult << "\n";
		}
		std::cout << "\n=== Benchmarks complete ===\n\n";
	}

	
	// draw two ImGui windows each frame
	void BenchmarkComponent::RenderImGui()
	{
		const int count = static_cast<int>(m_intResults.size());
		if (count == 0) return;

		// Build X-axis overlay string
		std::string xLabel;
		for (const auto& l : m_stepLabels)
			xLabel += l + " ";

		// Window 1: Exercise 1
		ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);

		ImGui::Begin("Exercise 1 - Integer array cache performance");

		ImGui::Text("Step sizes: %s", xLabel.c_str());

		ImGui::Text("Y axis: microseconds (averaged, outliers removed)");

		float maxVal = *std::max_element(m_intResults.begin(), m_intResults.end());
		ImGui::PlotLines("##int", m_intResults.data(), count,
			0, nullptr, 0.f, maxVal * 1.1f, ImVec2(460, 120));

		ImGui::Columns(2, "intcols", true);
		ImGui::Text("Step"); ImGui::NextColumn();
		ImGui::Text("Microseconds"); ImGui::NextColumn();

		ImGui::Separator();
		for (int i = 0; i < count; ++i)
		{
			ImGui::Text("%s", m_stepLabels[i].c_str()); ImGui::NextColumn();
			ImGui::Text("%.1f", m_intResults[i]);       ImGui::NextColumn();
		}
		ImGui::Columns(1);
		ImGui::End();

		// Window 2: Exercise 2
		ImGui::SetNextWindowSize(ImVec2(500, 280), ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2(10, 220), ImGuiCond_Once);
		ImGui::Begin("Exercise 2 - GameObject3D vs Alt (SoA)");

		ImGui::Text("Step sizes: %s", xLabel.c_str());
		ImGui::Text("Y axis: microseconds (averaged, outliers removed)");

		float maxGO = std::max(
			*std::max_element(m_go3dResults.begin(), m_go3dResults.end()),
			*std::max_element(m_go3dAltResults.begin(), m_go3dAltResults.end()));

		ImGui::Text("GameObject3D (AoS - slow):");
		ImGui::PlotLines("##go3d", m_go3dResults.data(), count,
			0, nullptr, 0.f, maxGO * 1.1f, ImVec2(460, 80));

		ImGui::Text("GameObject3DAlt (SoA - fast):");
		ImGui::PlotLines("##go3dalt", m_go3dAltResults.data(), count,
			0, nullptr, 0.f, maxGO * 1.1f, ImVec2(460, 80));

		ImGui::Columns(3, "gocols", true);
		ImGui::Text("Step"); ImGui::NextColumn();
		ImGui::Text("AoS (us)"); ImGui::NextColumn();
		ImGui::Text("SoA (us)"); ImGui::NextColumn();
		ImGui::Separator();
		for (int i = 0; i < count; ++i)
		{
			ImGui::Text("%s", m_stepLabels[i].c_str());  ImGui::NextColumn();
			ImGui::Text("%.1f", m_go3dResults[i]);        ImGui::NextColumn();
			ImGui::Text("%.1f", m_go3dAltResults[i]);     ImGui::NextColumn();
		}
		ImGui::Columns(1);
		ImGui::End();
	}
}