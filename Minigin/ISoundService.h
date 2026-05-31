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

		// Looping background music. Only one music track plays at a time;
		// PlayMusic replaces whatever was playing. StopMusic halts it.
		virtual void PlayMusic(const std::string& filePath, bool loop = true) = 0;
		virtual void StopMusic() = 0;
	};
}
