#pragma once

namespace dae
{
	class GameObject;

	class Component
	{
	public:
		virtual void Update(float) {}
		virtual void FixedUpdate(float) {}
		virtual void Render() const {}

		void MarkForDestroy() { m_MarkedForDestroy = true; }
		bool IsMarkedForDestroy()	const { return m_MarkedForDestroy; }

		GameObject* GetOwner() const { return m_pOwner; }

		explicit Component(GameObject* pOwner) : m_pOwner(pOwner) {}
		virtual ~Component() = default;
		Component(const Component&) = delete;
		Component(Component&&) = delete;
		Component& operator=(const Component&) = delete;
		Component& operator=(Component&&) = delete;

	private:
		GameObject* m_pOwner;
		bool m_MarkedForDestroy{ false };
	};
}