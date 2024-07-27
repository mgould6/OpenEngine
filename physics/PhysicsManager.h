#pragma once

#include <bullet/btBulletDynamicsCommon.h>
#include <vector>

class PhysicsManager {
public:
    PhysicsManager();
    ~PhysicsManager();

    void Initialize();
    void Update(float deltaTime);
    void AddRigidBody(btRigidBody* body);
    void SetGravity(const btVector3& gravity);

private:
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;
    std::vector<btRigidBody*> rigidBodies;
};
