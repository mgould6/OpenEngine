#include "ShaderManager.h"
#include "../common_utils/Logger.h"
#include "../common_utils/Utils.h"
#include <fstream>
#include <sstream>
#include <iostream>

Shader* ShaderManager::lightingShader = nullptr;
Shader* ShaderManager::depthShader = nullptr;
Shader* ShaderManager::postProcessingShader = nullptr;
Shader* ShaderManager::brightExtractShader = nullptr;
Shader* ShaderManager::blurShader = nullptr;
Shader* ShaderManager::combineShader = nullptr;
Shader* ShaderManager::ssaoShader = nullptr;
Shader* ShaderManager::toneMappingShader = nullptr;

Shader* ShaderManager::loadShader(const char* vertexPath, const char* fragmentPath) {
    Shader* shader = new Shader(vertexPath, fragmentPath);
    checkShaderCompileErrors(shader->ID, "PROGRAM");
    return shader;
}

Shader* ShaderManager::loadAndCompileShader(const char* vertexPath, const char* fragmentPath) {
    Shader* shader = new Shader(vertexPath, fragmentPath);
    if (!shader->isCompiled()) {
        Logger::log("Failed to compile shader: " + std::string(vertexPath) + " " + std::string(fragmentPath), Logger::ERROR);
        delete shader;
        return nullptr;
    }
    return shader;
}

bool ShaderManager::initShaders() {
    lightingShader = loadAndCompileShader("shaders/shadow/lighting_vertex_shader.vs", "shaders/shadow/lighting_fragment_shader.fs");
    depthShader = loadAndCompileShader("shaders/depth/depth_vertex_shader.vs", "shaders/depth/depth_fragment_shader.fs");
    postProcessingShader = loadAndCompileShader("shaders/post_processing/post_processing.vs", "shaders/post_processing/post_processing.fs");
    brightExtractShader = loadAndCompileShader("shaders/post_processing/bright_extract.vs", "shaders/post_processing/bright_extract.fs");
    blurShader = loadAndCompileShader("shaders/post_processing/blur.vs", "shaders/post_processing/blur.fs");
    combineShader = loadAndCompileShader("shaders/post_processing/combine.vs", "shaders/post_processing/combine.fs");
    ssaoShader = loadAndCompileShader("shaders/post_processing/ssao.vs", "shaders/post_processing/ssao.fs");
    toneMappingShader = loadAndCompileShader("shaders/post_processing/tone_mapping.vs", "shaders/post_processing/tone_mapping.fs");

    if (!lightingShader || !depthShader || !postProcessingShader || !brightExtractShader ||
        !blurShader || !combineShader || !ssaoShader || !toneMappingShader) {
        Logger::log("Failed to initialize shaders", Logger::ERROR);
        return false;
    }
    return true;
}


void ShaderManager::checkShaderCompileErrors(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}

bool ShaderManager::fileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}
