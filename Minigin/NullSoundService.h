#pragma once
#include "ISoundService.h"

namespace dae
{
	// Default no-op sound service used when no real service is registered
	class NullSoundService final : public ISoundService
	{
	public:
		void PlaySound(const std::string&, int) override {}
		void StopAllSounds() override {}
	};
}
