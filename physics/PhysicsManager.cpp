#include "PhysicsManager.h"
#include "../common_utils/Logger.h"

PhysicsManager::PhysicsManager() :
    collisionConfiguration(nullptr),
    dispatcher(nullptr),
    overlappingPairCache(nullptr),
    solver(nullptr),
    dynamicsWorld(nullptr) {
}

PhysicsManager::~PhysicsManager() {
    for (btRigidBody* body : rigidBodies) {
        if (dynamicsWorld) dynamicsWorld->removeRigidBody(body);
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

    if (!dynamicsWorld) {
        Logger::log("Failed to initialize dynamicsWorld.", Logger::ERROR);
    }
    else {
        Logger::log("PhysicsManager initialized successfully.", Logger::INFO);
        dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
    }
}


void PhysicsManager::Update(float deltaTime) {
    if (!dynamicsWorld) {
        Logger::log("PhysicsManager::Update: dynamicsWorld is null.", Logger::ERROR);
        return;
    }

    dynamicsWorld->stepSimulation(deltaTime);

    for (btRigidBody* body : rigidBodies) {
        if (!body) continue; // Skip null bodies
        // Update transformations for rendering if necessary
    }
}

void PhysicsManager::AddRigidBody(btRigidBody* body) {
    if (dynamicsWorld && body) {
        dynamicsWorld->addRigidBody(body);
        rigidBodies.push_back(body);
        Logger::log("Rigid body added to PhysicsManager.", Logger::INFO);
    }
    else {
        Logger::log("Failed to add rigid body: dynamicsWorld or body is null.", Logger::ERROR);
    }
}

void PhysicsManager::SetGravity(const btVector3& gravity) {
    if (dynamicsWorld) {
        dynamicsWorld->setGravity(gravity);
    }
    else {
        Logger::log("Cannot set gravity: dynamicsWorld is null.", Logger::ERROR);
    }
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
