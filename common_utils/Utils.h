#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <string>

void checkGLError(const std::string& message);
void checkShaderCompileErrors(unsigned int shader, const std::string& type);

#endif // UTILS_H
