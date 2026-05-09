#pragma once
#include <string>

namespace dae
{
	class ISoundService
	{
	public:
		virtual ~ISoundService() = default;

		virtual void PlaySound(const std::string& filePath, int volume = 128) = 0;
		virtual void StopAllSounds() = 0;
	};
}
