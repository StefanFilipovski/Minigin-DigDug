#pragma once
#include <memory>
#include <string>
#include "Component.h"

namespace dae
{
	class Texture2D;
	class TransformComponent;

	class RenderComponent final : public Component
	{
	public:
		explicit RenderComponent(GameObject* pOwner) : Component(pOwner) {}

		void Render() const override;
		void SetTexture(const std::string& filename);
		void SetTexture(std::shared_ptr<Texture2D> texture);

	private:
		std::shared_ptr<Texture2D> m_Texture{};
	};
}