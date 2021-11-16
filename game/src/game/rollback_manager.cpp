#include <array>
#include <game/game_globals.h>
#include <game/rollback_manager.h>
#include <game/game_manager.h>
#include <cassert>
#include <utils/log.h>
#include <fmt/format.h>

namespace game
{

    RollbackManager::RollbackManager(GameManager& gameManager, core::EntityManager& entityManager) :
        gameManager_(gameManager), entityManager_(entityManager),
        currentTransformManager_(entityManager),
        currentPhysicsManager_(entityManager), currentPlayerManager_(entityManager, currentPhysicsManager_, gameManager_),
        lastValidatePhysicsManager_(entityManager),
        lastValidatePlayerManager_(entityManager, lastValidatePhysicsManager_, gameManager_)
		{
        for (auto& input : inputs_)
        {
            std::fill(input.begin(), input.end(), 0u);
        }
        currentPhysicsManager_.RegisterTriggerListener(*this);
    }

    void RollbackManager::SimulateToCurrentFrame()
    {
        const auto currentFrame = gameManager_.GetCurrentFrame();
        const auto lastValidateFrame = gameManager_.GetLastValidateFrame();
        //Destroying all created Entities after the last validated frame
        for (const auto& createdEntity : createdEntities_)
        {
            if (createdEntity.createdFrame > lastValidateFrame)
            {
                core::LogDebug(fmt::format("Destroy entity{}", createdEntity.entity));
                entityManager_.DestroyEntity(createdEntity.entity);
            }
        }
        createdEntities_.clear();
        //Remove DESTROY flags
        for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
        {
            if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
            {
                entityManager_.RemoveComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED));
                if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::BALLOON)))
                {
                    balloonEntity_ = entity;
                }
            }
        }

        //Revert the current game state to the last validated game state
        currentPhysicsManager_.CopyAllComponents(lastValidatePhysicsManager_);
        currentPlayerManager_.CopyAllComponents(lastValidatePlayerManager_.GetAllComponents());

        for (Frame frame = lastValidateFrame + 1; frame <= currentFrame; frame++)
        {
            testedFrame_ = frame;
            //Copy player inputs to player manager
            for (PlayerNumber playerNumber = 0; playerNumber < maxPlayerNmb; playerNumber++)
            {
                const auto playerInput = GetInputAtFrame(playerNumber, frame);
                const auto playerEntity = gameManager_.GetEntityFromPlayerNumber(playerNumber);
                if(playerEntity == core::EntityManager::INVALID_ENTITY)
                {
                    core::LogWarning(fmt::format("Invalid Entity in {}:line {}", __FILE__, __LINE__));
                    continue;
                }
                auto playerCharacter = currentPlayerManager_.GetComponent(playerEntity);
                playerCharacter.input = playerInput;
                currentPlayerManager_.SetComponent(playerEntity, playerCharacter);
            }
            //Simulate one frame of the game
            currentPlayerManager_.FixedUpdate(sf::seconds(GameManager::FixedPeriod));
            currentPhysicsManager_.FixedUpdate(sf::seconds(GameManager::FixedPeriod));
        }
        //Copy the physics states to the transforms
        for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
        {
            if (!entityManager_.HasComponent(entity,
                                             static_cast<core::EntityMask>(core::ComponentType::BODY2D) |
                                             static_cast<core::EntityMask>(core::ComponentType::TRANSFORM)))
                continue;
            const auto& body = currentPhysicsManager_.GetBody(entity);
            currentTransformManager_.SetPosition(entity, body.position);
        }
    }
    void RollbackManager::SetPlayerInput(PlayerNumber playerNumber, PlayerInput playerInput, std::uint32_t inputFrame)
    {
        //Should only be called on the server
        if (currentFrame_ < inputFrame)
        {
            StartNewFrame(inputFrame);
        }
        inputs_[playerNumber][currentFrame_ - inputFrame] = playerInput;
        if (lastReceivedFrame_[playerNumber] < inputFrame)
        {
            lastReceivedFrame_[playerNumber] = inputFrame;
            //Repeat the same inputs until currentFrame
            for (size_t i = 0; i < currentFrame_ - inputFrame; i++)
            {
                inputs_[playerNumber][i] = playerInput;
            }
        }
    }

    void RollbackManager::StartNewFrame(Frame newFrame)
    {
        if (currentFrame_ > newFrame)
            return;
        const auto delta = newFrame - currentFrame_;
        if (delta == 0)
        {
            return;
        }
        for (auto& inputs : inputs_)
        {
            for (auto i = inputs.size() - 1; i >= delta; i--)
            {
                inputs[i] = inputs[i - delta];
            }

            for (Frame i = 0; i < delta; i++)
            {
                inputs[i] = inputs[delta];
            }
        }
        currentFrame_ = newFrame;
    }

    void RollbackManager::ValidateFrame(Frame newValidateFrame)
    {
        const auto lastValidateFrame = gameManager_.GetLastValidateFrame();
        //Destroying all created Entities after the last validated frame
        for (const auto& createdEntity : createdEntities_)
        {
            if (createdEntity.createdFrame > lastValidateFrame)
            {
                core::LogDebug(fmt::format("Destroy entity{}", createdEntity.entity));
                entityManager_.DestroyEntity(createdEntity.entity);
            }
        }
        createdEntities_.clear();
        //Remove DESTROYED flag
        for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
        {
            if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
            {
                entityManager_.RemoveComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED));
                if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::BALLOON)))
                {
                    balloonEntity_ = entity;
                }
            }

        }
        createdEntities_.clear();

        //We check that we got all the inputs
        for (PlayerNumber playerNumber = 0; playerNumber < maxPlayerNmb; playerNumber++)
        {
            if (GetLastReceivedFrame(playerNumber) < newValidateFrame)
            {
                assert(false && "We should not validate a frame if we did not receive all inputs!!!");
                return;
            }
        }
        //We use the current game state as the temporary new validate game state
        currentPhysicsManager_.CopyAllComponents(lastValidatePhysicsManager_);
        currentPlayerManager_.CopyAllComponents(lastValidatePlayerManager_.GetAllComponents());

        //We simulate the frames until the new validated frame
        for (Frame frame = lastValidateFrame_ + 1; frame <= newValidateFrame; frame++)
        {
            testedFrame_ = frame;
            //Copy the players inputs into the player manager
            for (PlayerNumber playerNumber = 0; playerNumber < maxPlayerNmb; playerNumber++)
            {
                const auto playerInput = GetInputAtFrame(playerNumber, frame);
                const auto playerEntity = gameManager_.GetEntityFromPlayerNumber(playerNumber);
                auto playerCharacter = currentPlayerManager_.GetComponent(playerEntity);
                playerCharacter.input = playerInput;
                currentPlayerManager_.SetComponent(playerEntity, playerCharacter);
            }
            //We simulate one frame
            currentPlayerManager_.FixedUpdate(sf::seconds(GameManager::FixedPeriod));
            currentPhysicsManager_.FixedUpdate(sf::seconds(GameManager::FixedPeriod));
        }
        //Definitely remove DESTROY entities
        for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
        {
            if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
            {
                entityManager_.DestroyEntity(entity);
            }
        }
        //Copy back the new validate game state to the last validated game state
        lastValidatePlayerManager_.CopyAllComponents(currentPlayerManager_.GetAllComponents());
        lastValidatePhysicsManager_.CopyAllComponents(currentPhysicsManager_);
        lastValidateFrame_ = newValidateFrame;
        createdEntities_.clear();
    }
    void RollbackManager::ConfirmFrame(Frame newValidateFrame, const std::array<PhysicsState, maxPlayerNmb>& serverPhysicsState)
    {
        ValidateFrame(newValidateFrame);
        for (PlayerNumber playerNumber = 0; playerNumber < maxPlayerNmb; playerNumber++)
        {
            const PhysicsState lastPhysicsState = GetValidatePhysicsState(playerNumber);
            if (serverPhysicsState[playerNumber] != lastPhysicsState)
            {
                assert(false && "Physics State are not equal");
            }
        }
    }
    PhysicsState RollbackManager::GetValidatePhysicsState(PlayerNumber playerNumber) const
    {
        PhysicsState state = 0;
        const core::Entity playerEntity = gameManager_.GetEntityFromPlayerNumber(playerNumber);
        const auto& playerBody = lastValidatePhysicsManager_.GetBody(playerEntity);

        const auto pos = playerBody.position;
        const auto* posPtr = reinterpret_cast<const PhysicsState*>(&pos);
        //Adding position
        for (size_t i = 0; i < sizeof(core::Vec2f) / sizeof(PhysicsState); i++)
        {
            state += posPtr[i];
        }

        //Adding velocity
        const auto velocity = playerBody.velocity;
        const auto* velocityPtr = reinterpret_cast<const PhysicsState*>(&velocity);
        for (size_t i = 0; i < sizeof(core::Vec2f) / sizeof(PhysicsState); i++)
        {
            state += velocityPtr[i];
        }
       
        return state;
    }

    PhysicsState RollbackManager::GetValidatePhysicsStateBalloon() const
    {
        PhysicsState state = 0;

        const auto& balloonBody = lastValidatePhysicsManager_.GetBody(balloonEntity_);

        const auto pos = balloonBody.position;

        const auto* posPtr = reinterpret_cast<const PhysicsState*>(&pos);

        // Adding position.
        for (size_t i = 0; i < sizeof(core::Vec2f) / sizeof(PhysicsState); i++)
        {
            state += posPtr[i];
        }

        // Velocity.
        const auto velocity = balloonBody.velocity;
        const auto* velocityPtr = reinterpret_cast<const PhysicsState*>(&velocity);
        for (size_t i = 0; i < sizeof(core::Vec2f) / sizeof(PhysicsState); i++)
        {
            state += velocityPtr[i];
        }
        return state;
    }

    void RollbackManager::SpawnPlayer(PlayerNumber playerNumber, core::Entity entity, core::Vec2f position)
    {
        CircleBody playerBody;
        playerBody.position = position;
        Box playerBox;
        playerBox.extends = core::Vec2f::one() * 0.5f;

        PlayerCharacter playerCharacter;
        playerCharacter.playerNumber = playerNumber;

        currentPlayerManager_.AddComponent(entity);
        currentPlayerManager_.SetComponent(entity, playerCharacter);

        currentPhysicsManager_.AddBody(entity);
        currentPhysicsManager_.SetBody(entity, playerBody);
        currentPhysicsManager_.AddBox(entity);
        currentPhysicsManager_.SetBox(entity, playerBox);

        lastValidatePlayerManager_.AddComponent(entity);
        lastValidatePlayerManager_.SetComponent(entity, playerCharacter);

        lastValidatePhysicsManager_.AddBody(entity);
        lastValidatePhysicsManager_.SetBody(entity, playerBody);
        lastValidatePhysicsManager_.AddBox(entity);
        lastValidatePhysicsManager_.SetBox(entity, playerBox);

        currentTransformManager_.AddComponent(entity);
        currentTransformManager_.SetPosition(entity, position);
       
    }

    PlayerInput RollbackManager::GetInputAtFrame(PlayerNumber playerNumber, Frame frame)
    {
        assert(currentFrame_ - frame < inputs_[playerNumber].size() &&
            "Trying to get input too far in the past");
        return inputs_[playerNumber][currentFrame_ - frame];
    }

    void RollbackManager::OnTrigger(core::Entity entity1, core::Entity entity2)
    {
        std::function<void(const PlayerCharacter&, core::Entity, core::Entity)> ManageCollision =
            [this](const auto& player, auto playerEntity, auto balloonEntity)
        {
            //lower health point
            auto playerCharacter = currentPlayerManager_.GetComponent(playerEntity);
            if (playerCharacter.invincibilityTime <= 0.0f)
            {
                core::LogDebug(fmt::format("Player {} is hit by balloon", playerCharacter.playerNumber));
                playerCharacter.health--;
                playerCharacter.invincibilityTime = playerInvincibilityPeriod;
                gameManager_.DestroyBalloon(balloonEntity_);
                gameManager_.SpawnBalloon(balloonSpawnPosition, balloonSpawnVelocity);
            }
            currentPlayerManager_.SetComponent(playerEntity, playerCharacter);

        };
        if (entityManager_.HasComponent(entity1, static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)) &&
            entityManager_.HasComponent(entity2, static_cast<core::EntityMask>(ComponentType::BALLOON)))
        {
            const auto& player = currentPlayerManager_.GetComponent(entity1);
            ManageCollision(player, entity1, entity2);

        }
        if (entityManager_.HasComponent(entity2, static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)) &&
            entityManager_.HasComponent(entity1, static_cast<core::EntityMask>(ComponentType::BALLOON)))
        {
            const auto& player = currentPlayerManager_.GetComponent(entity2);
            ManageCollision(player, entity2, entity1);
        }
    }

    void RollbackManager::SpawnBalloon(core::Entity entity, core::Vec2f position, core::Vec2f velocity)
    {
        createdEntities_.push_back({ entity, testedFrame_ });
        balloonEntity_ = entity;
        CircleBody balloonBody;
        balloonBody.position = position;
        balloonBody.velocity = velocity;
        balloonBody.rebound = .80f;

        currentPhysicsManager_.AddBody(entity);
        currentPhysicsManager_.SetBody(entity, balloonBody);

    	lastValidatePhysicsManager_.AddBody(entity);
        lastValidatePhysicsManager_.SetBody(entity, balloonBody);

        currentTransformManager_.AddComponent(entity);
        currentTransformManager_.SetPosition(entity, position);
        currentTransformManager_.SetScale(entity, core::Vec2f::one() * balloonScale);

    }

    void RollbackManager::DestroyEntity(core::Entity entity)
    {
        //we don't need to save a balloon that has been created in the time window
        if (std::find_if(createdEntities_.begin(), createdEntities_.end(), [entity](auto newEntity)
            {
                return newEntity.entity == entity;
            }) != createdEntities_.end())
        {
            entityManager_.DestroyEntity(entity);
            return;
        }
        entityManager_.AddComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED));
    }
}
