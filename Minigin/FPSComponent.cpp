#include "FPSComponent.h"
#include "TextComponent.h"
#include "GameObject.h"
#include <string>
#include <iomanip>
#include <sstream>

void dae::FPSComponent::Update(float deltaTime)
{
	m_AccumulatedTime += deltaTime;
	++m_FrameCount;

	if (m_AccumulatedTime >= 1.0f)
	{
		const float fps = m_FrameCount / m_AccumulatedTime;

		auto* text = GetOwner()->GetComponent<TextComponent>();
		if (text)
		{
			std::ostringstream ss;
			ss << std::fixed << std::setprecision(1) << fps << " FPS";
			text->SetText(ss.str());
		}

		m_AccumulatedTime = 0.f;
		m_FrameCount = 0;
	}
}