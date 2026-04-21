#pragma once
#include "GameWorld.h"

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Collision/ContactListener.h"
#include "Jolt/Physics/Body/Body.h"
#include <Jolt/Physics/PhysicsSystem.h>

namespace JD
{
	class WorldContactListener : public JPH::ContactListener
	{
	public:
		WorldContactListener(JD::Gameworld& world, JPH::PhysicsSystem& physics)
			: world(world), physics(physics) {
		}

		// See: ContactListener
		virtual JPH::ValidateResult	OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override
		{
			// std::cout << "Contact validate callback" << std::endl;

			// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
			return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
		}

		virtual void			OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
		{
			//std::cout << "A contact was added" << std::endl;

			// If one body is a sensor, keep the other body awake so the contact isn't lost

			// As an idea here, still have a game object class that can be attached to an entity.
			// Then check if bodies have the game object component and if so call thir on collision functions.

			if (inBody1.IsSensor()) {
				physics.GetBodyInterfaceNoLock().ActivateBody(inBody2.GetID());
			}
			if (inBody2.IsSensor()) {
				physics.GetBodyInterfaceNoLock().ActivateBody(inBody1.GetID());
			}
		}

		virtual void			OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
		{
			// Keep the dynamic body awake while it remains inside the sensor
			if (inBody1.IsSensor()) {
				physics.GetBodyInterfaceNoLock().ActivateBody(inBody2.GetID());
			}
			if (inBody2.IsSensor()) {
				physics.GetBodyInterfaceNoLock().ActivateBody(inBody1.GetID());
			}
		}

		virtual void			OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
		{
			// std::cout << "A contact was removed" << std::endl;

		}

	private:
		JD::Gameworld& world;
		JPH::PhysicsSystem& physics;
	};
}
