#include "game/balloon_manager.h"

namespace game
{
	BalloonManager::BalloonManager(core::EntityManager& entityManager, GameManager& gameManager, PhysicsManager& physicsManager):
	ComponentManager(entityManager), gameManager_(gameManager), physicsManager_(physicsManager)
	{
	}

	void BalloonManager::FixedUpdate(sf::Time dt)
	{
		for (core::Entity entity = 0; entity <entityManager_.GetEntitiesSize(); entity++)
		{
			if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::BALLOON)))
			{
				
			}
		}
	}

}