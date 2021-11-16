#pragma once

#include "engine/component.h"
#include "engine/entity.h"
#include "maths/angle.h"
#include "maths/vec2.h"

#include <SFML/System/Time.hpp>

#include "utils/action_utility.h"

namespace game
{
    enum class BodyType
    {
        DYNAMIC,
        STATIC
    };
    struct CircleBody
    {
        core::Vec2f position = core::Vec2f::zero();
        core::Vec2f velocity = core::Vec2f::zero();
        BodyType bodyType = BodyType::DYNAMIC;
    	static float CalculateDistance(CircleBody body1, CircleBody body2);
        float rebound = 0.9f;
        static constexpr float radius = 0.17f;
    };

    struct Box
    {
        core::Vec2f extends = core::Vec2f::one();
        bool isTrigger = false;
    };

    class OnTriggerInterface
    {
    public:
        virtual ~OnTriggerInterface() = default;
        virtual void OnTrigger(core::Entity entity1, core::Entity entity2) = 0;
    };

    class CircleManager : public core::ComponentManager<CircleBody, static_cast<core::EntityMask>(core::ComponentType::BODY2D)>
    {
    public:
        using ComponentManager::ComponentManager;
    };
    class BoxManager : public core::ComponentManager<Box, static_cast<core::EntityMask>(core::ComponentType::BOX_COLLIDER2D)>
    {
    public:
        using ComponentManager::ComponentManager;
    };

    class PhysicsManager
    {
    public:
        explicit PhysicsManager(core::EntityManager& entityManager);
        void FixedUpdate(sf::Time dt);
        [[nodiscard]] const CircleBody& GetBody(core::Entity entity) const;
        void SetBody(core::Entity entity, const CircleBody& body);
        void AddBody(core::Entity entity);

        void AddBox(core::Entity entity);
        void SetBox(core::Entity entity, const Box& box);
        [[nodiscard]] const Box& GetBox(core::Entity entity) const;

        void RegisterTriggerListener(OnTriggerInterface& collisionInterface);
        void CopyAllComponents(const PhysicsManager& physicsManager);
    private:
        core::EntityManager& entityManager_;
        CircleManager circleManager_;
        BoxManager boxManager_;
        core::Action<core::Entity, core::Entity> onTriggerAction_;

        bool BodyContact(CircleBody body1, CircleBody body2);
        void ResolveBodyContact(CircleBody& body1, CircleBody& body2);
        core::Vec2f ContactPoint(const CircleBody& rb1, const CircleBody& rb2) const;
        core::Vec2f RelocatedCenter(const CircleBody& body, const core::Vec2f& v);
    };

}
