#include "ShaderManager.h"
#include "../common_utils/Logger.h"
#include "../common_utils/Utils.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Static shader pointers
Shader* ShaderManager::lightingShader = nullptr;
Shader* ShaderManager::depthShader = nullptr;
Shader* ShaderManager::postProcessingShader = nullptr;
Shader* ShaderManager::brightExtractShader = nullptr;
Shader* ShaderManager::blurShader = nullptr;
Shader* ShaderManager::combineShader = nullptr;
Shader* ShaderManager::ssaoShader = nullptr;
Shader* ShaderManager::toneMappingShader = nullptr; // Add Tone Mapping shader

Shader* ShaderManager::loadShader(const char* vertexPath, const char* fragmentPath) {
    Shader* shader = new Shader(vertexPath, fragmentPath);
    checkShaderCompileErrors(shader->ID, "PROGRAM");
    return shader;
}

bool ShaderManager::initShaders() {
    // Verify shader files exist
    std::string shaderPaths[] = {
        "shaders/shadow/lighting_vertex_shader.vs", "shaders/shadow/lighting_fragment_shader.fs",
        "shaders/depth/depth_vertex_shader.vs", "shaders/depth/depth_fragment_shader.fs",
        "shaders/post_processing/post_processing.vs", "shaders/post_processing/post_processing.fs",
        "shaders/post_processing/bright_extract.vs", "shaders/post_processing/bright_extract.fs",
        "shaders/post_processing/blur.vs", "shaders/post_processing/blur.fs",
        "shaders/post_processing/combine.vs", "shaders/post_processing/combine.fs",
        "shaders/post_processing/ssao.vs", "shaders/post_processing/ssao.fs", // Add SSAO shaders
        "shaders/post_processing/tone_mapping.vs", "shaders/post_processing/tone_mapping.fs" // Add Tone Mapping shaders
    };

    for (const auto& path : shaderPaths) {
        if (!fileExists(path)) {
            std::cerr << "Error: Shader file not found at: " << path << std::endl;
            return false;
        }
    }

    // Initialize shaders
    lightingShader = loadShader("shaders/shadow/lighting_vertex_shader.vs", "shaders/shadow/lighting_fragment_shader.fs");
    depthShader = loadShader("shaders/depth/depth_vertex_shader.vs", "shaders/depth/depth_fragment_shader.fs");
    postProcessingShader = loadShader("shaders/post_processing/post_processing.vs", "shaders/post_processing/post_processing.fs");
    brightExtractShader = loadShader("shaders/post_processing/bright_extract.vs", "shaders/post_processing/bright_extract.fs");
    blurShader = loadShader("shaders/post_processing/blur.vs", "shaders/post_processing/blur.fs");
    combineShader = loadShader("shaders/post_processing/combine.vs", "shaders/post_processing/combine.fs");
    ssaoShader = loadShader("shaders/post_processing/ssao.vs", "shaders/post_processing/ssao.fs"); // Initialize SSAO shader
    toneMappingShader = loadShader("shaders/post_processing/tone_mapping.vs", "shaders/post_processing/tone_mapping.fs"); // Initialize Tone Mapping shader

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
