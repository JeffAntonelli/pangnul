#include <game/physics_manager.h>

namespace game
{

    PhysicsManager::PhysicsManager(core::EntityManager& entityManager) :
        bodyManager_(entityManager), boxManager_(entityManager), entityManager_(entityManager)
    {

    }

    bool Box2Box(float r1x, float r1y, float r1w, float r1h, float r2x, float r2y, float r2w, float r2h) //Intersect.
    {
        return r1x + r1w >= r2x &&    // r1 right edge past r2 left
            r1x <= r2x + r2w &&    // r1 left edge past r2 right      Intersect.
            r1y + r1h >= r2y &&    // r1 top edge past r2 bottom
            r1y <= r2y + r2h;
    }

    void PhysicsManager::FixedUpdate(sf::Time dt)
    {
        for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
        {
            if (!entityManager_.HasComponent(entity, static_cast<core::EntityMask>(core::ComponentType::BODY2D)))
                continue;
            auto body = bodyManager_.GetComponent(entity);
            core::Vec2f Gravity = { 0, -9.81 };
            core::Vec2f max_pos = { (core::windowSize.x / core::pixelPerMeter / 2),
                (core::windowSize.y / core::pixelPerMeter / 2) };
            core::Vec2f min_pos = { -(core::windowSize.x / core::pixelPerMeter / 2),
            -(core::windowSize.y / core::pixelPerMeter / 2) };

            body.velocity += Gravity * dt.asSeconds();
            body.position += body.velocity * dt.asSeconds();

            if (body.position.x <= min_pos.x + core::radius)
            {
                body.position.x = min_pos.x + core::radius;
                body.velocity.x = -CircleBody::Rebound * body.velocity.x;
            }
            if (body.position.y <= min_pos.y + core::radius)           
            {
                body.position.y = min_pos.y + core::radius;
                body.velocity.y = -CircleBody::Rebound * body.velocity.y;
            }
            if (body.position.x >= max_pos.x - core::radius)
            {
                body.position.x = max_pos.x - core::radius;
                body.velocity.x = -CircleBody::Rebound * body.velocity.x;
            }
            if (body.position.y >= max_pos.y - core::radius)
            {
                body.position.y = max_pos.y - core::radius;
                body.velocity.y = -CircleBody::Rebound * body.velocity.y;
            }

            bodyManager_.SetComponent(entity, body);

            auto body1 = bodyManager_.GetComponent(0);
            auto body2 = bodyManager_.GetComponent(1);

            if(BodyContact(body1, body2))
            {
                ResolveBodyContact(body1, body2);
                bodyManager_.SetComponent(0, body1);
                bodyManager_.SetComponent(1, body2);
            }

        }
        for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
        {
            if (!entityManager_.HasComponent(entity,
                                                   static_cast<core::EntityMask>(core::ComponentType::BODY2D) |
                                                   static_cast<core::EntityMask>(core::ComponentType::BOX_COLLIDER2D)) ||
                entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
                continue;
            for (core::Entity otherEntity = entity; otherEntity < entityManager_.GetEntitiesSize(); otherEntity++)
            {
                if (entity == otherEntity)
                    continue;
                if (!entityManager_.HasComponent(otherEntity,
                                                 static_cast<core::EntityMask>(core::ComponentType::BODY2D) | static_cast<core::EntityMask>(core::ComponentType::BOX_COLLIDER2D)) ||
                    entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
                    continue;
                const CircleBody& body1 = bodyManager_.GetComponent(entity);
                const Box& box1 = boxManager_.GetComponent(entity);

                const CircleBody& body2 = bodyManager_.GetComponent(otherEntity);
                const Box& box2 = boxManager_.GetComponent(otherEntity);

              /*  if (Box2Box(
                    body1.position.x - box1.extends.x,
                    body1.position.y - box1.extends.y,
                    box1.extends.x * 2.0f,
                    box1.extends.y * 2.0f,
                    body2.position.x - box2.extends.x,
                    body2.position.y - box2.extends.y,
                    box2.extends.x * 2.0f,
                    box2.extends.y * 2.0f))
                {
                    onTriggerAction_.Execute(entity, otherEntity);
                }*/

            }
        }
    }

    void PhysicsManager::SetBody(core::Entity entity, const CircleBody& body)
    {
        bodyManager_.SetComponent(entity, body);
    }

    const CircleBody& PhysicsManager::GetBody(core::Entity entity) const
    {
        return bodyManager_.GetComponent(entity);
    }

    void PhysicsManager::AddBody(core::Entity entity)
    {
        bodyManager_.AddComponent(entity);
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
        bodyManager_.CopyAllComponents(physicsManager.bodyManager_.GetAllComponents());
        boxManager_.CopyAllComponents(physicsManager.boxManager_.GetAllComponents());
    }

    bool PhysicsManager::BodyContact(CircleBody body1, CircleBody body2)
    {
        return CircleBody::CalculateDistance(body1, body2) < (core::radius + core::radius);
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
            body1.position, ContactPoint(body1, body2)).x * v1t * -core::forceRestitution;
        body1.velocity.y = ComputeNormal(body1.position, ContactPoint(body1, body2)).y * v2n + ComputeTangent(
            body1.position, ContactPoint(body1, body2)).y * v1t * -core::forceRestitution;
        body2.velocity.x = ComputeNormal(body2.position, ContactPoint(body1, body2)).x * v1n + ComputeTangent(
            body2.position, ContactPoint(body1, body2)).x * v2t * -core::forceRestitution;
        body2.velocity.y = ComputeNormal(body2.position, ContactPoint(body1, body2)).y * v1n + ComputeTangent(
            body2.position, ContactPoint(body1, body2)).y * v2t * -core::forceRestitution;


        body1.position = RelocatedCenter(body1, ContactPoint(body1, body2));
        body2.position = RelocatedCenter(body2, ContactPoint(body1, body2));
        body1.velocity = body1.velocity * -core::forceRestitution;
        body2.velocity = body2.velocity * -core::forceRestitution;
    }

    core::Vec2f PhysicsManager::ContactPoint(const CircleBody& body1, const CircleBody& body2) const
    {
        double ratio = (core::radius) / ((core::radius)+(core::radius));
        return (body2.position - body1.position) * ratio + body1.position;
    }

    core::Vec2f PhysicsManager::RelocatedCenter(const CircleBody& body, const core::Vec2f& v)
    {
        double ratio = (core::radius) / (body.position - v).GetMagnitude();
        return (body.position - v) * ratio + v;
    }


    float CircleBody::CalculateDistance(CircleBody body1, CircleBody body2)
    {
        const float dx = body2.position.x - body1.position.x;
        const float dy = body2.position.y - body1.position.y;
        return std::sqrt(dx * dx + dy * dy);
    }
}
