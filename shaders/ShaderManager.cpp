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
Shader* ShaderManager::boneShader = nullptr;

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
    Logger::log("DEBUG: Initializing shaders...", Logger::INFO);

    boneShader = loadAndCompileShader("shaders/bone/bone_vertex_shader.vs", "shaders/bone/bone_fragment_shader.fs");
    if (boneShader && boneShader->isCompiled()) {
        allShaders.push_back(boneShader);
        Logger::log("DEBUG: Bone shader compiled successfully.", Logger::INFO);
    }
    else {
        Logger::log("ERROR: Bone shader failed to compile!", Logger::ERROR);
    }

    // Load a fallback shader
    if (!boneShader || !boneShader->isCompiled()) {
        Logger::log("WARNING: Bone shader failed! Loading fallback shader.", Logger::WARNING);
        boneShader = loadAndCompileShader("shaders/shadow/lighting_vertex_shader.vs", "shaders/shadow/lighting_fragment_shader.fs");
        if (!boneShader || !boneShader->isCompiled()) {
            Logger::log("ERROR: Fallback shader also failed!", Logger::ERROR);
            return false;
        }
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
