#include "ShaderManager.h"
#include "Logger.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Static shader pointers
Shader* ShaderManager::lightingShader = nullptr;
Shader* ShaderManager::depthShader = nullptr;
Shader* ShaderManager::debugDepthQuad = nullptr;
Shader* ShaderManager::postProcessingShader = nullptr;
Shader* ShaderManager::brightExtractShader = nullptr;
Shader* ShaderManager::blurShader = nullptr;
Shader* ShaderManager::combineShader = nullptr;

Shader* ShaderManager::loadShader(const char* vertexPath, const char* fragmentPath) {
    Shader* shader = new Shader(vertexPath, fragmentPath);
    checkShaderCompileErrors(shader->ID, "PROGRAM");
    return shader;
}

void ShaderManager::initShaders() {
    // Verify shader files exist
    std::string shaderPaths[] = {
        "lighting_vertex_shader.vs", "lighting_fragment_shader.fs",
        "depth_vertex_shader.vs", "depth_fragment_shader.fs",
        "depth_debug.vs", "depth_debug.fs",
        "post_processing.vs", "post_processing.fs",
        "bright_extract.vs", "bright_extract.fs",
        "blur.vs", "blur.fs",
        "combine.vs", "combine.fs"
    };

    for (const auto& path : shaderPaths) {
        if (!fileExists(path)) {
            std::cerr << "Error: Shader file not found at: " << path << std::endl;
            return;
        }
    }

    // Initialize shaders
    lightingShader = loadShader("lighting_vertex_shader.vs", "lighting_fragment_shader.fs");
    depthShader = loadShader("depth_vertex_shader.vs", "depth_fragment_shader.fs");
    debugDepthQuad = loadShader("depth_debug.vs", "depth_debug.fs");
    postProcessingShader = loadShader("post_processing.vs", "post_processing.fs");
    brightExtractShader = loadShader("bright_extract.vs", "bright_extract.fs");
    blurShader = loadShader("blur.vs", "blur.fs");
    combineShader = loadShader("combine.vs", "combine.fs");
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
