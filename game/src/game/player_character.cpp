#include <game/player_character.h>
#include <game/game_manager.h>

namespace game
{
    PlayerCharacterManager::PlayerCharacterManager(core::EntityManager& entityManager, PhysicsManager& physicsManager, GameManager& gameManager) :
        ComponentManager(entityManager),
        physicsManager_(physicsManager),
        gameManager_(gameManager)

    {

    }

    void PlayerCharacterManager::FixedUpdate(sf::Time dt)
    {
        for (core::Entity playerEntity = 0; playerEntity < entityManager_.GetEntitiesSize(); playerEntity++)
        {
            if (!entityManager_.HasComponent(playerEntity,
            static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)))
                continue;
            auto playerBody = physicsManager_.GetBody(playerEntity);
            auto playerCharacter = GetComponent(playerEntity);
            const auto input = playerCharacter.input;

            const bool right = input & PlayerInputEnum::PlayerInput::RIGHT;
            const bool left = input & PlayerInputEnum::PlayerInput::LEFT;
            const bool up = input & PlayerInputEnum::PlayerInput::UP;
            //const bool down = input & PlayerInputEnum::PlayerInput::DOWN;// Enlever

            //const auto angularVelocity = ((left ? -1.0f : 0.0f) + (right ? 1.0f : 0.0f)) * playerAngularSpeed;

            //playerBody.angularVelocity = angularVelocity;

            auto dir = core::Vec2f::up(); //Enlever.
            //dir = dir.Rotate(-(playerBody.rotation + playerBody.angularVelocity * dt.asSeconds()));// Enlever.
            auto dir_l = core::Vec2f::left();
            
            const auto acceleration = ((up ? -0.5f : 0.0f) + (up ? 0.5f : 0.0f)) * dir; //Enlever.
            const auto acceleration_l = ((left ? 0.0f : -2.0f) + (right ? 0.0f : +2.0f)) * dir_l;

            //playerBody.velocity += acceleration * dt.asSeconds(); // Enlever.
            playerBody.velocity += acceleration + acceleration_l * dt.asSeconds();

            physicsManager_.SetBody(playerEntity, playerBody);

            if (playerCharacter.invincibilityTime > 0.0f)
            {
                playerCharacter.invincibilityTime -= dt.asSeconds();
                SetComponent(playerEntity, playerCharacter);
            }
            //Check if cannot shoot, and increase shootingTime
            if (playerCharacter.shootingTime < playerShootingPeriod)
            {
                playerCharacter.shootingTime += dt.asSeconds();
                SetComponent(playerEntity, playerCharacter);
            }
            //Shooting mechanism
            if (playerCharacter.shootingTime >= playerShootingPeriod)
            {
                if (input & PlayerInputEnum::PlayerInput::SHOOT)
                {
                    const auto currentPlayerSpeed = playerBody.velocity.GetMagnitude();
                    const auto bulletVelocity = dir *
                        ((core::Vec2f::Dot(playerBody.velocity, dir) > 0.0f ? currentPlayerSpeed : 0.0f)
                            + bulletSpeed);
                    const auto bulletPosition = playerBody.position + dir * 0.5f + playerBody.velocity * dt.asSeconds();
                    gameManager_.SpawnBullet(playerCharacter.playerNumber,
                        bulletPosition,
                        bulletVelocity);
                    playerCharacter.shootingTime = 0.0f;
                    SetComponent(playerEntity, playerCharacter);
                }
            }
        }
    }
}
