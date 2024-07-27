#include "Globals.h"
#include "render_utils/Renderer.h"

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
Model* myModel = nullptr;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
unsigned int depthMapFBO[Renderer::NUM_CASCADES];
unsigned int hdrFBO;
unsigned int depthMap[Renderer::NUM_CASCADES];
unsigned int colorBuffers[2];
unsigned int pingpongFBO[2];
unsigned int pingpongBuffer[2];
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;
unsigned int VAO;
unsigned int VBO;
