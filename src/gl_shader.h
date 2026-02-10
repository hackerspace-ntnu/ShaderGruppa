#pragma once
#include <glad/glad.h>
#include <vector>

GLuint compileShader(GLenum type, const char* source, const char* label);
GLuint linkProgram(const std::vector<GLuint>& shaders, const char* label);
GLuint buildProgramFromFiles(const char* vertPath, const char* fragPath);
GLuint buildComputeProgramFromFile(const char* compPath);