#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include "Shader.h"

class ShaderManager {
public:
    static Shader* lightingShader;
    static Shader* depthShader;
    static Shader* postProcessingShader;
    static Shader* brightExtractShader;
    static Shader* blurShader;
    static Shader* combineShader;

    static bool initShaders();
    static Shader* loadShader(const char* vertexPath, const char* fragmentPath);

private:
    static void checkShaderCompileErrors(unsigned int shader, const std::string& type);
    static bool fileExists(const std::string& path);
};

#endif // SHADER_MANAGER_H
