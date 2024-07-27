#ifndef GLOBALS_H
#define GLOBALS_H

#include "model/Camera.h"
#include "model/Model.h"

extern Camera camera;
extern Model* myModel;
extern float deltaTime;
extern float lastFrame;
extern unsigned int depthMapFBO[];
extern unsigned int hdrFBO;
extern unsigned int depthMap[];
extern unsigned int colorBuffers[];
extern unsigned int pingpongFBO[];
extern unsigned int pingpongBuffer[];
extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;
extern unsigned int VAO;
extern unsigned int VBO;

#endif // GLOBALS_H
