#include "Globals.h"
#include "../render_utils/Renderer.h"
#include "../model/Model.h"
#include "../animation/AnimationController.h"

Model* myModel = nullptr;
AnimationController* animationController = nullptr;

// Define other global variables

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
