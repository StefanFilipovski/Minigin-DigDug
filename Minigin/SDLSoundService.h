#pragma once
#include "ISoundService.h"
#include <memory>

namespace dae
{
	// Real sound service backed by SDL_mixer
	// Uses pimpl so no SDL_mixer headers leak into client code
	class SDLSoundService final : public ISoundService
	{
	public:
		SDLSoundService();
		~SDLSoundService() override;

		SDLSoundService(const SDLSoundService&) = delete;
		SDLSoundService& operator=(const SDLSoundService&) = delete;

		void PlaySound(const std::string& filePath, int volume = 128) override;
		void StopAllSounds() override;

	private:
		class Impl;
		std::unique_ptr<Impl> m_pImpl;
	};
}
