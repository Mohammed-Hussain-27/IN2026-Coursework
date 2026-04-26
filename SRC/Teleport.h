#ifndef __TELEPORT_H__
#define __TELEPORT_H__

#include "GameObject.h"
#include "GameObjectType.h"

class Teleport : public GameObject
{
public:
	// Constructor - creates a teleport power-up at a random position
	Teleport(void) : GameObject("Teleport")
	{
		// Set a random position on screen
		mPosition.x = (rand() % 200) - 100;
		mPosition.y = (rand() % 200) - 100;
		mPosition.z = 0.0;

		// No velocity - power-up stays still
		mVelocity.x = 0.0;
		mVelocity.y = 0.0;
		mVelocity.z = 0.0;
	}

	~Teleport(void) {}

	// Only collide with the spaceship
	bool CollisionTest(shared_ptr<GameObject> o)
	{
		if (o->GetType() != GameObjectType("Spaceship")) return false;
		if (mBoundingShape.get() == NULL) return false;
		if (o->GetBoundingShape().get() == NULL) return false;
		return mBoundingShape->CollisionTest(o->GetBoundingShape());
	}

	// When collected by spaceship - remove from world
	void OnCollision(const GameObjectList& objects)
	{
		for (auto& obj : objects)
		{
			if (obj->GetType() == GameObjectType("Spaceship"))
			{
				mWorld->FlagForRemoval(GetThisPtr());
				return;
			}
		}
	}
};

#endif