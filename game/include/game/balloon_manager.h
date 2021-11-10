#pragma once

#include <SFML/System/Time.hpp>

#include "game_globals.h"
#include "physics_manager.h"

namespace game
{
	struct Balloon
	{
		PlayerNumber playerNumber = INVALID_PLAYER;
	};

	class GameManager;
	class BalloonManager : public core::ComponentManager<Balloon, static_cast<core::EntityMask>(ComponentType::BALLOON)>
	{
	public:
		explicit BalloonManager(core::EntityManager& entityManager, GameManager& gameManager, PhysicsManager& physicsManager);
		void FixedUpdate(sf::Time dt);
	private:
		GameManager& gameManager_;
		PhysicsManager& physicsManager_;
	};
}