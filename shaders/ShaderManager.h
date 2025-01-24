#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include "Shader.h"
#include <vector>
#include <string>

class ShaderManager {
public:
    // Static shader pointers
    static Shader* lightingShader;
    static Shader* shadowShader;

    static Shader* depthShader;
    static Shader* postProcessingShader;
    static Shader* brightExtractShader;
    static Shader* blurShader;
    static Shader* combineShader;
    static Shader* ssaoShader;
    static Shader* toneMappingShader;

    // Container for all shaders
    static std::vector<Shader*> allShaders;

    // Methods
    static Shader* loadAndCompileShader(const char* vertexPath, const char* fragmentPath);
    static bool initShaders();
    static Shader* loadShader(const char* vertexPath, const char* fragmentPath);

private:
    static void checkShaderCompileErrors(unsigned int shader, const std::string& type);
    static bool fileExists(const std::string& path);
};

#endif // SHADER_MANAGER_H
