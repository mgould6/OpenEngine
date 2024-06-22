#ifndef GUIMANAGER_H
#define GUIMANAGER_H

#include <GLFW/glfw3.h>

class GuiManager {
public:
    static void init(GLFWwindow* window);
    static void shutdown();
    static void newFrame();
    static void render();
};

#endif // GUIMANAGER_H
