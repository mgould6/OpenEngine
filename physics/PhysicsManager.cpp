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

btRigidBody* PhysicsManager::CreateCube(float size, float mass, const btVector3& position) {
    btCollisionShape* cubeShape = new btBoxShape(btVector3(size, size, size));
    btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), position));
    btVector3 inertia(0, 0, 0);
    if (mass != 0.0f) cubeShape->calculateLocalInertia(mass, inertia);
    btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, cubeShape, inertia);
    btRigidBody* body = new btRigidBody(rigidBodyCI);
    AddRigidBody(body);
    return body;
}

btRigidBody* PhysicsManager::CreatePlane(const btVector3& normal, float constant) {
    btCollisionShape* planeShape = new btStaticPlaneShape(normal, constant);
    btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
    btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(0.0f, motionState, planeShape, btVector3(0, 0, 0));
    btRigidBody* body = new btRigidBody(rigidBodyCI);
    AddRigidBody(body);
    return body;
}
