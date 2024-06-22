#ifndef PHYSICSMANAGER_H
#define PHYSICSMANAGER_H

class PhysicsManager {
public:
    static PhysicsManager& instance() {
        static PhysicsManager instance;
        return instance;
    }
    void init();
    void update(float deltaTime);

private:
    PhysicsManager() = default;
    ~PhysicsManager() = default;
    PhysicsManager(const PhysicsManager&) = delete;
    PhysicsManager& operator=(const PhysicsManager&) = delete;
};

#endif // PHYSICSMANAGER_H
