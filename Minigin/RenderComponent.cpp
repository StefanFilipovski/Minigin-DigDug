#include "RenderComponent.h"
#include "TransformComponent.h"
#include "ResourceManager.h"
#include "Renderer.h"
#include "GameObject.h"

void dae::RenderComponent::Render() const
{
	if (!m_Texture) return;

	auto* transform = GetOwner()->GetComponent<TransformComponent>();
	if (!transform) return;

	const auto& pos = transform->GetPosition();
	Renderer::GetInstance().RenderTexture(*m_Texture, pos.x, pos.y);
}

void dae::RenderComponent::SetTexture(const std::string& filename)
{
	m_Texture = ResourceManager::GetInstance().LoadTexture(filename);
}

void dae::RenderComponent::SetTexture(std::shared_ptr<Texture2D> texture)
{
	m_Texture = texture;
}