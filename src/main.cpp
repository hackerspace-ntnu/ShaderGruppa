// src/main.cpp
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static std::string readTextFile(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error(std::string("Failed to open file: ") + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum type, const char* source, const char* label) {
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
        throw std::runtime_error("Shader compile failed");
    }
    return s;
}

static GLuint linkProgram(const std::vector<GLuint>& shaders, const char* label) {
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
        throw std::runtime_error("Program link failed");
    }

    for (auto s : shaders) glDetachShader(p, s);
    return p;
}

static void glfwErrorCallback(int code, const char* msg) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, msg);
}

static GLuint createTextureRGBA8(int w, int h) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // Immutable storage is supported in GL 4.2+, so it's fine for GL 4.3.
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

static void resizeTextureRGBA8(GLuint tex, int w, int h) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
    glBindTexture(GL_TEXTURE_2D, 0);
}

int main() {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to init GLFW\n");
        return 1;
    }

    // Request OpenGL 4.3 core (compute shaders require 4.3+)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(960, 540, "OpenGL 4.3 Compute Minimal", nullptr, nullptr);
    if (!win) {
        std::fprintf(stderr, "Failed to create window (GPU/driver may not support GL 4.3)\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    // GLAD (glad.h-style) loader
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "Failed to load OpenGL via glad\n");
        glfwDestroyWindow(win);
        glfwTerminate();
        return 1;
    }

    std::fprintf(stdout, "GL Vendor:   %s\n", glGetString(GL_VENDOR));
    std::fprintf(stdout, "GL Renderer: %s\n", glGetString(GL_RENDERER));
    std::fprintf(stdout, "GL Version:  %s\n", glGetString(GL_VERSION));

    int texW = 960, texH = 540;
    GLuint tex = createTextureRGBA8(texW, texH);

    GLuint progCompute = 0, progBlit = 0;
    try {
        const auto compSrc = readTextFile("shaders/compute.comp");
        const auto vertSrc = readTextFile("shaders/fullscreen.vert");
        const auto fragSrc = readTextFile("shaders/blit.frag");

        {
            GLuint cs = compileShader(GL_COMPUTE_SHADER, compSrc.c_str(), "compute.comp");
            progCompute = linkProgram({cs}, "compute");
            glDeleteShader(cs);
        }

        {
            GLuint vs = compileShader(GL_VERTEX_SHADER, vertSrc.c_str(), "fullscreen.vert");
            GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc.c_str(), "blit.frag");
            progBlit = linkProgram({vs, fs}, "blit");
            glDeleteShader(vs);
            glDeleteShader(fs);
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Shader build error: %s\n", e.what());
        glDeleteTextures(1, &tex);
        glfwDestroyWindow(win);
        glfwTerminate();
        return 1;
    }

    // Uniforms
    const GLint uTimeLoc = glGetUniformLocation(progCompute, "uTime");
    const GLint uTexLoc  = glGetUniformLocation(progBlit, "uTex");

    // Core profile requires a VAO bound for glDrawArrays
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(win, &fbW, &fbH);
        if (fbW <= 0 || fbH <= 0) {
            continue; // minimized
        }

        if (fbW != texW || fbH != texH) {
            texW = fbW;
            texH = fbH;
            resizeTextureRGBA8(tex, texW, texH);
        }

        glViewport(0, 0, fbW, fbH);

        const float t = (float)glfwGetTime();

        // Compute pass: write into texture via imageStore
        glUseProgram(progCompute);
        if (uTimeLoc >= 0) glUniform1f(uTimeLoc, t);

        glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

        const GLuint gx = (GLuint)((texW + 15) / 16);
        const GLuint gy = (GLuint)((texH + 15) / 16);
        glDispatchCompute(gx, gy, 1);

        // Ensure compute writes are visible to texture sampling
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        // Render pass: sample texture and draw fullscreen triangle
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(progBlit);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        if (uTexLoc >= 0) glUniform1i(uTexLoc, 0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(win);
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &tex);
    glDeleteProgram(progCompute);
    glDeleteProgram(progBlit);

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
