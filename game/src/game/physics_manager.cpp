#include <game/physics_manager.h>

namespace game
{

    PhysicsManager::PhysicsManager(core::EntityManager& entityManager) :
        circleManager_(entityManager),
		boxManager_(entityManager),
		entityManager_(entityManager)
    {

    }

    void PhysicsManager::FixedUpdate(sf::Time dt)
    {
        for (core::Entity entity1 = 0; entity1 < entityManager_.GetEntitiesSize(); entity1++)
        {
            if (!entityManager_.HasComponent(entity1, static_cast<core::EntityMask>(core::ComponentType::BODY2D)))
                continue;
            auto body1 = circleManager_.GetComponent(entity1);
            core::Vec2f Gravity = { 0, -9.81 };
            core::Vec2f max_pos = { (core::windowSize.x / core::pixelPerMeter / 2),
                (core::windowSize.y / core::pixelPerMeter / 2) };
            core::Vec2f min_pos = { -(core::windowSize.x / core::pixelPerMeter / 2),
            -(core::windowSize.y / core::pixelPerMeter / 2) };

            body1.velocity += Gravity * dt.asSeconds();
            body1.position += body1.velocity * dt.asSeconds();

            if (body1.position.x <= min_pos.x + body1.radius)
            {
                body1.position.x = min_pos.x + body1.radius;
                body1.velocity.x = -body1.rebound * body1.velocity.x;
            }
            if (body1.position.y <= min_pos.y + body1.radius)           
            {
                body1.position.y = min_pos.y + body1.radius;
                body1.velocity.y = -body1.rebound * body1.velocity.y;
            }
            if (body1.position.x >= max_pos.x - body1.radius)
            {
                body1.position.x = max_pos.x - body1.radius;
                body1.velocity.x = -body1.rebound * body1.velocity.x;
            }
            if (body1.position.y >= max_pos.y - CircleBody::radius)
            {
                body1.position.y = max_pos.y - CircleBody::radius;
                body1.velocity.y = -body1.rebound * body1.velocity.y;
            }

            circleManager_.SetComponent(entity1, body1);


        }
        for (core::Entity entity1 = 0; entity1 < entityManager_.GetEntitiesSize(); entity1++)
        {
            if (!entityManager_.HasComponent(entity1, static_cast<core::EntityMask>(core::ComponentType::BODY2D)))
                continue;
            auto body1 = circleManager_.GetComponent(entity1);

            for (core::Entity entity2 = entity1 + 1; entity2 < entityManager_.GetEntitiesSize(); entity2++)
            {
                if (!entityManager_.HasComponent(entity2, static_cast<core::EntityMask>(core::ComponentType::BODY2D)))
                    continue;
                auto body2 = circleManager_.GetComponent(entity2);

                if (BodyContact(body1, body2))
                {
                	ResolveBodyContact(body1, body2);
                	onTriggerAction_.Execute(entity1, entity2);
                    circleManager_.SetComponent(entity1, body1);
                    circleManager_.SetComponent(entity2, body2);
                }
            }
        }
    }

    void PhysicsManager::SetBody(core::Entity entity, const CircleBody& body)
    {
        circleManager_.SetComponent(entity, body);
    }

    const CircleBody& PhysicsManager::GetBody(core::Entity entity) const
    {
        return circleManager_.GetComponent(entity);
    }

    void PhysicsManager::AddBody(core::Entity entity)
    {
        circleManager_.AddComponent(entity);
    }

    void PhysicsManager::AddBox(core::Entity entity)
    {
        boxManager_.AddComponent(entity);
    }

    void PhysicsManager::SetBox(core::Entity entity, const Box& box)
    {
        boxManager_.SetComponent(entity, box);
    }

    const Box& PhysicsManager::GetBox(core::Entity entity) const
    {
        return boxManager_.GetComponent(entity);
    }

    void PhysicsManager::RegisterTriggerListener(OnTriggerInterface& collisionInterface)
    {
        onTriggerAction_.RegisterCallback(
            [&collisionInterface](core::Entity entity1, core::Entity entity2) { collisionInterface.OnTrigger(entity1, entity2); });
    }

    void PhysicsManager::CopyAllComponents(const PhysicsManager& physicsManager)
    {
        circleManager_.CopyAllComponents(physicsManager.circleManager_.GetAllComponents());
        boxManager_.CopyAllComponents(physicsManager.boxManager_.GetAllComponents());
    }

    bool PhysicsManager::BodyContact(CircleBody body1, CircleBody body2)
    {
        return CircleBody::CalculateDistance(body1, body2) < (CircleBody::radius  + CircleBody::radius);
    }

    void PhysicsManager::ResolveBodyContact(CircleBody& body1, CircleBody& body2)
    {
        float v1n = ComputeNormal(body1.position, ContactPoint(body1, body2)).x * body1.velocity.x +
            ComputeNormal(body1.position, ContactPoint(body1, body2)).y * body1.velocity.y;
        float v1t = ComputeTangent(body1.position, ContactPoint(body1, body2)).x * body1.velocity.x +
            ComputeTangent(body1.position, ContactPoint(body1, body2)).y * body1.velocity.y;
        float v2n = ComputeNormal(body2.position, ContactPoint(body1, body2)).x * body2.velocity.x +
            ComputeNormal(body2.position, ContactPoint(body1, body2)).y * body2.velocity.y;
        float v2t = ComputeTangent(body2.position, ContactPoint(body1, body2)).x * body2.velocity.x +
            ComputeTangent(body2.position, ContactPoint(body1, body2)).y * body2.velocity.y;

        body1.velocity.x = ComputeNormal(body1.position, ContactPoint(body1, body2)).x * v2n + ComputeTangent(
            body1.position, ContactPoint(body1, body2)).x * v1t * -body1.rebound;
        body1.velocity.y = ComputeNormal(body1.position, ContactPoint(body1, body2)).y * v2n + ComputeTangent(
            body1.position, ContactPoint(body1, body2)).y * v1t * -body1.rebound;
        body2.velocity.x = ComputeNormal(body2.position, ContactPoint(body1, body2)).x * v1n + ComputeTangent(
            body2.position, ContactPoint(body1, body2)).x * v2t * -body2.rebound;
        body2.velocity.y = ComputeNormal(body2.position, ContactPoint(body1, body2)).y * v1n + ComputeTangent(
            body2.position, ContactPoint(body1, body2)).y * v2t * -body2.rebound;


        body1.position = RelocatedCenter(body1, ContactPoint(body1, body2));
        body2.position = RelocatedCenter(body2, ContactPoint(body1, body2));
        body1.velocity = body1.velocity * -body1.rebound;
        body2.velocity = body2.velocity * -body2.rebound;
    }

    core::Vec2f PhysicsManager::ContactPoint(const CircleBody& body1, const CircleBody& body2) const
    {
        double ratio = (CircleBody::radius) / ((CircleBody::radius)+(CircleBody::radius));
        return (body2.position - body1.position) * ratio + body1.position;
    }

    core::Vec2f PhysicsManager::RelocatedCenter(const CircleBody& body, const core::Vec2f& v)
    {
        double ratio = (CircleBody::radius) / (body.position - v).GetMagnitude();
        return (body.position - v) * ratio + v;
    }


    float CircleBody::CalculateDistance(CircleBody body1, CircleBody body2)
    {
        const float dx = body2.position.x - body1.position.x;
        const float dy = body2.position.y - body1.position.y;
        return std::sqrt(dx * dx + dy * dy);
    }
}
