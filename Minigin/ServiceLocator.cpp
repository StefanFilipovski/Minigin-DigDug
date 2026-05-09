#include "ServiceLocator.h"
#include "NullSoundService.h"

namespace dae
{
	std::unique_ptr<ISoundService> ServiceLocator::m_pSoundService{};
	std::unique_ptr<ISoundService> ServiceLocator::m_pDefaultService = std::make_unique<NullSoundService>();

	ISoundService& ServiceLocator::GetSoundService()
	{
		return m_pSoundService ? *m_pSoundService : *m_pDefaultService;
	}

	void ServiceLocator::RegisterSoundService(std::unique_ptr<ISoundService> service)
	{
		m_pSoundService = std::move(service);
	}
}
