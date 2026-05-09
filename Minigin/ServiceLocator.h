#pragma once
#include "ISoundService.h"
#include <memory>

namespace dae
{
	class ServiceLocator final
	{
	public:
		static ISoundService& GetSoundService();
		static void RegisterSoundService(std::unique_ptr<ISoundService> service);

	private:
		static std::unique_ptr<ISoundService> m_pSoundService;
		static std::unique_ptr<ISoundService> m_pDefaultService;
	};
}
