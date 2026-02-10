// src/main.cpp
#include <cstdio>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "gl_shader.h"
#include "gl_texture.h"

static void glfwErrorCallback(int code, const char* msg) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, msg);
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
        progCompute = buildComputeProgramFromFile("shaders/compute.comp");
        progBlit    = buildProgramFromFiles("shaders/fullscreen.vert", "shaders/blit.frag");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Shader build error: %s\n", e.what());
        glDeleteTextures(1, &tex);
        glfwDestroyWindow(win);
        glfwTerminate();
        return 1;
    }

    const GLint uTimeLoc = glGetUniformLocation(progCompute, "uTime");
    const GLint uTexLoc  = glGetUniformLocation(progBlit, "uTex");

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(win, &fbW, &fbH);
        if (fbW <= 0 || fbH <= 0) continue;

        if (fbW != texW || fbH != texH) {
            texW = fbW;
            texH = fbH;
            resizeTextureRGBA8(tex, texW, texH);
        }

        glViewport(0, 0, fbW, fbH);

        const float t = (float)glfwGetTime();

        glUseProgram(progCompute);
        if (uTimeLoc >= 0) glUniform1f(uTimeLoc, t);

        glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

        const GLuint gx = (GLuint)((texW + 15) / 16);
        const GLuint gy = (GLuint)((texH + 15) / 16);
        glDispatchCompute(gx, gy, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

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
