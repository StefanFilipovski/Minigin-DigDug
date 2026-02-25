#include "RenderComponent.h"
#include "TransformComponent.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "GameObject.h"

namespace dae
{
	void RenderComponent::Render() const
	{
		if (!m_Texture) return;

		auto* transform = GetOwner()->GetComponent<TransformComponent>();
		if (!transform) return;

		// Use world position so children are rendered relative to their parent
		const auto& pos = transform->GetWorldPosition();
		Renderer::GetInstance().RenderTexture(*m_Texture, pos.x, pos.y);
	}

	void RenderComponent::SetTexture(const std::string& filename)
	{
		m_Texture = ResourceManager::GetInstance().LoadTexture(filename);
	}

	void RenderComponent::SetTexture(std::shared_ptr<Texture2D> texture)
	{
		m_Texture = texture;
	}
}