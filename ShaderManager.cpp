#include "ShaderManager.h"
#include "Logger.h"
#include "Utils.h"

Shader* ShaderManager::lightingShader = nullptr;
Shader* ShaderManager::depthShader = nullptr;
Shader* ShaderManager::debugDepthQuad = nullptr;
Shader* ShaderManager::postProcessingShader = nullptr;

Shader* ShaderManager::loadShader(const char* vertexPath, const char* fragmentPath) {
    Shader* shader = new Shader(vertexPath, fragmentPath);
    checkShaderCompileErrors(shader->ID, "PROGRAM");
    return shader;
}

void ShaderManager::initShaders() {
    lightingShader = loadShader("lighting_vertex_shader.vs", "lighting_fragment_shader.fs");
    depthShader = loadShader("depth_vertex_shader.vs", "depth_fragment_shader.fs");
    debugDepthQuad = loadShader("depth_debug.vs", "depth_debug.fs");
    postProcessingShader = loadShader("post_processing.vs", "post_processing.fs");
}
