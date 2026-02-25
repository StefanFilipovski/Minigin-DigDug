#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "Component.h"

namespace dae
{
	class GameObject final
	{
	public:
		void Update(float deltaTime);
		void FixedUpdate(float fixedTimeStep);
		void Render() const;

		void MarkForDestroy() { m_MarkedForDestroy = true; }
		bool IsMarkedForDestroy() const { return m_MarkedForDestroy; }
		~GameObject();

		
		// Scene Graph
		void SetParent(GameObject* parent, bool keepWorldPosition = false);

		GameObject* GetParent()    const { return m_parent; }
		size_t      GetChildCount() const { return m_children.size(); }
		GameObject* GetChildAt(size_t index) const { return m_children[index]; }

		
		//Component management
		
		template<typename T, typename... Args>
		T* AddComponent(Args&&... args)
		{
			static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
			auto component = std::make_unique<T>(this, std::forward<Args>(args)...);
			T* ptr = component.get();
			m_Components.emplace_back(std::move(component));
			return ptr;
		}

		template<typename T>
		void RemoveComponent()
		{
			auto it = std::find_if(m_Components.begin(), m_Components.end(),
				[](const auto& c) { return dynamic_cast<T*>(c.get()) != nullptr; });
			if (it != m_Components.end())
				(*it)->MarkForDestroy();
		}

		template<typename T>
		T* GetComponent() const
		{
			for (const auto& c : m_Components)
				if (auto* casted = dynamic_cast<T*>(c.get()))
					return casted;
			return nullptr;
		}

		template<typename T>
		bool HasComponent() const { return GetComponent<T>() != nullptr; }

		GameObject() = default;
		GameObject(const GameObject&) = delete;
		GameObject(GameObject&&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject& operator=(GameObject&&) = delete;

	private:
		// Low-level helpers — only SetParent should call these
		void AddChild(GameObject* child);
		void RemoveChild(GameObject* child);

		// Returns true if 'candidate' is a descendant of this object (or this itself)
		bool IsDescendant(const GameObject* candidate) const;

		std::vector<std::unique_ptr<Component>> m_Components{};
		bool m_MarkedForDestroy{ false };

		GameObject* m_parent{};
		std::vector<GameObject*> m_children{};
	};
}