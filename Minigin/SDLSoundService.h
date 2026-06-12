#pragma once
#include "ISoundService.h"
#include <memory>

namespace dae
{
	
	class SDLSoundService final : public ISoundService
	{
	public:
		SDLSoundService();
		~SDLSoundService() override;

		SDLSoundService(const SDLSoundService&) = delete;
		SDLSoundService& operator=(const SDLSoundService&) = delete;

		void PlaySound(const std::string& filePath, int volume = 128) override;
		void StopAllSounds() override;
		void PlayMusic(const std::string& filePath, bool loop = true) override;
		void StopMusic() override;
		void SetMuted(bool muted) override;

	private:
		class Impl;
		std::unique_ptr<Impl> m_pImpl;
	};
}
