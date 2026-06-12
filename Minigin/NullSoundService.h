#pragma once
#include "ISoundService.h"

namespace dae
{
	class NullSoundService final : public ISoundService
	{
	public:
		void PlaySound(const std::string&, int) override {}
		void StopAllSounds() override {}
		void PlayMusic(const std::string&, bool) override {}
		void StopMusic() override {}
		void SetMuted(bool) override {}
	};
}
