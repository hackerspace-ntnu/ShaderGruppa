#include "gl_shader.h"
#include "file_io.h"

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

GLuint compileShader(GLenum type, const char* source, const char* label) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &source, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log((size_t)len + 1);
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::fprintf(stderr, "Shader compile failed (%s):\n%s\n", label, log.data());
        glDeleteShader(s);
        throw std::runtime_error(std::string("Shader compile failed: ") + label);
    }
    return s;
}

GLuint linkProgram(const std::vector<GLuint>& shaders, const char* label) {
    GLuint p = glCreateProgram();
    for (auto s : shaders) glAttachShader(p, s);
    glLinkProgram(p);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log((size_t)len + 1);
        glGetProgramInfoLog(p, len, nullptr, log.data());
        std::fprintf(stderr, "Program link failed (%s):\n%s\n", label, log.data());
        glDeleteProgram(p);
        throw std::runtime_error(std::string("Program link failed: ") + label);
    }

    for (auto s : shaders) glDetachShader(p, s);
    return p;
}

GLuint buildProgramFromFiles(const char* vertPath, const char* fragPath) {
    const std::string vertSrc = readTextFile(vertPath);
    const std::string fragSrc = readTextFile(fragPath);

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc.c_str(), vertPath);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc.c_str(), fragPath);
    GLuint p  = linkProgram({vs, fs}, fragPath);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

GLuint buildComputeProgramFromFile(const char* compPath) {
    const std::string compSrc = readTextFile(compPath);
    GLuint cs = compileShader(GL_COMPUTE_SHADER, compSrc.c_str(), compPath);
    GLuint p  = linkProgram({cs}, "compute");
    glDeleteShader(cs);
    return p;
}
