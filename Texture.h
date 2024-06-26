#pragma once
#include <GL/glew.h>  // Ensure GLEW is included first

#include <string>
#include <GL/glew.h>
#include <stb_image.h>

class TextureClass {
public:
    unsigned int id;
    std::string type;
    std::string path;

    // Constructor
    TextureClass(const std::string& path, const std::string& directory, const std::string& typeName);

    // Bind the texture
    void bind() const;

private:
    void loadTexture(const std::string& path, const std::string& directory);
};
