#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include "Shader.h"

class ShaderManager {
public:
    static Shader* loadShader(const char* vertexPath, const char* fragmentPath);
    static void initShaders();

    static Shader* lightingShader;
    static Shader* depthShader;
    static Shader* debugDepthQuad;
    static Shader* postProcessingShader;
};

#endif // SHADERMANAGER_H
