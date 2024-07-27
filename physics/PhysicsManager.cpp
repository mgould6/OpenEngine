#include "PhysicsManager.h"

PhysicsManager::PhysicsManager() :
    collisionConfiguration(nullptr),
    dispatcher(nullptr),
    overlappingPairCache(nullptr),
    solver(nullptr),
    dynamicsWorld(nullptr) {
}

PhysicsManager::~PhysicsManager() {
    for (btRigidBody* body : rigidBodies) {
        dynamicsWorld->removeRigidBody(body);
        delete body->getMotionState();
        delete body;
    }
    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;
}

void PhysicsManager::Initialize() {
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    overlappingPairCache = new btDbvtBroadphase();
    solver = new btSequentialImpulseConstraintSolver();
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
}

void PhysicsManager::Update(float deltaTime) {
    dynamicsWorld->stepSimulation(deltaTime);
    for (btRigidBody* body : rigidBodies) {
        // Update transformations for rendering
    }
}

void PhysicsManager::AddRigidBody(btRigidBody* body) {
    dynamicsWorld->addRigidBody(body);
    rigidBodies.push_back(body);
}

void PhysicsManager::SetGravity(const btVector3& gravity) {
    dynamicsWorld->setGravity(gravity);
}
