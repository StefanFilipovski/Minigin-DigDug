#pragma once

namespace dae
{
	class GameObject;

	// abstract base
	class Command
	{
	public:
		virtual ~Command() = default;
		virtual void Execute() = 0;
	};

	// Commands for gameactor
	class GameActorCommand : public Command
	{
	public:
		explicit GameActorCommand(GameObject* pGameObject)
			: m_pGameObject(pGameObject) {
		}
		virtual ~GameActorCommand() = default;

	protected:
		GameObject* GetGameActor() const { return m_pGameObject; }

	private:
		GameObject* m_pGameObject;
	};
}