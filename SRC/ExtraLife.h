#ifndef __EXTRALIFE_H__
#define __EXTRALIFE_H__

#include "GameObject.h"
#include "GameObjectType.h"

class ExtraLife : public GameObject
{
public:
    // Track if properly collected by spaceship
    bool mCollectedBySpaceship;

    // Constructor - creates an extra life power-up at a random position
    ExtraLife(void) : GameObject("ExtraLife"), mCollectedBySpaceship(false)
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

    ~ExtraLife(void) {}

    // Check for collision with other objects
    bool CollisionTest(shared_ptr<GameObject> o)
    {
        // Only collide with the spaceship
        if (o->GetType() != GameObjectType("Spaceship")) return false;
        if (mBoundingShape.get() == NULL) return false;
        if (o->GetBoundingShape().get() == NULL) return false;
        return mBoundingShape->CollisionTest(o->GetBoundingShape());
    }

    // When collected - only flag if spaceship collected it
    void OnCollision(const GameObjectList& objects)
    {
        for (auto& obj : objects)
        {
            if (obj->GetType() == GameObjectType("Spaceship"))
            {
                // Mark as properly collected
                mCollectedBySpaceship = true;
                mWorld->FlagForRemoval(GetThisPtr());
                return;
            }
        }
    }
};

#endif