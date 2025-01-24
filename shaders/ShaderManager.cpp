#include "ShaderManager.h"
#include "../common_utils/Logger.h"
#include "../common_utils/Utils.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Static shader definitions
Shader* ShaderManager::lightingShader = nullptr;
Shader* ShaderManager::shadowShader = nullptr;

Shader* ShaderManager::depthShader = nullptr;
Shader* ShaderManager::postProcessingShader = nullptr;
Shader* ShaderManager::brightExtractShader = nullptr;
Shader* ShaderManager::blurShader = nullptr;
Shader* ShaderManager::combineShader = nullptr;
Shader* ShaderManager::ssaoShader = nullptr;
Shader* ShaderManager::toneMappingShader = nullptr;
std::vector<Shader*> ShaderManager::allShaders;

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
    if (!allShaders.empty()) {
        Logger::log("Shaders already initialized.", Logger::WARNING);
        return true;
    }

    // Load and store shaders
    lightingShader = loadAndCompileShader("shaders/shadow/lighting_vertex_shader.vs", "shaders/shadow/lighting_fragment_shader.fs");
    if (lightingShader) allShaders.push_back(lightingShader);

    //shadowShader = loadAndCompileShader("shaders/shadow/shadow_vertex_shader.vs", "shaders/shadow/shadow_fragment_shader.fs");
    //if (lightingShader) allShaders.push_back(lightingShader);

    //depthShader = loadAndCompileShader("shaders/depth/depth_vertex_shader.vs", "shaders/depth/depth_fragment_shader.fs");
    //if (depthShader) allShaders.push_back(depthShader);

    //postProcessingShader = loadAndCompileShader("shaders/post_processing/post_processing.vs", "shaders/post_processing/post_processing.fs");
    //if (postProcessingShader) allShaders.push_back(postProcessingShader);

    //brightExtractShader = loadAndCompileShader("shaders/post_processing/bright_extract.vs", "shaders/post_processing/bright_extract.fs");
    //if (brightExtractShader) allShaders.push_back(brightExtractShader);

    //blurShader = loadAndCompileShader("shaders/post_processing/blur.vs", "shaders/post_processing/blur.fs");
    //if (blurShader) allShaders.push_back(blurShader);

    //combineShader = loadAndCompileShader("shaders/post_processing/combine.vs", "shaders/post_processing/combine.fs");
    //if (combineShader) allShaders.push_back(combineShader);

    //ssaoShader = loadAndCompileShader("shaders/post_processing/ssao.vs", "shaders/post_processing/ssao.fs");
    //if (ssaoShader) allShaders.push_back(ssaoShader);

    //toneMappingShader = loadAndCompileShader("shaders/post_processing/tone_mapping.vs", "shaders/post_processing/tone_mapping.fs");
    //if (toneMappingShader) allShaders.push_back(toneMappingShader);

    if (allShaders.empty()) {
        Logger::log("No shaders initialized.", Logger::ERROR);
        return false;
    }

    Logger::log("Shaders initialized successfully.", Logger::INFO);
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
