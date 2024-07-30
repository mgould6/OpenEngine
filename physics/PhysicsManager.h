// PhysicsManager.h
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
    btRigidBody* CreateCube(float size, float mass, const btVector3& position);
    btRigidBody* CreatePlane(const btVector3& normal, float constant);
    std::vector<btRigidBody*> rigidBodies;

private:
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;
};
