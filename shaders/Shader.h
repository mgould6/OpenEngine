#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    unsigned int ID;

    // Constructor
    Shader(const char* vertexPath, const char* fragmentPath);

    // Use the shader program
    void use() const;

    // Uniform setters
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

    // Utility function to check if a uniform exists
    bool hasUniform(const std::string& name) const;

    // Check if the shader was compiled successfully
    bool isCompiled() const;

    void setMat4Array(const std::string& name, const std::vector<glm::mat4>& matrices) const;


private:
    bool compiled;
    void checkCompileErrors(unsigned int shader, const std::string& type) const;
};

#endif // SHADER_H
