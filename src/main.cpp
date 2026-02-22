// src/main.cpp
#include <cstdio>
#include <iostream>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "gl_shader.h"
#include "gl_texture.h"
#include "menu.h"

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
        // Show shader menu
        vector<std::string> shaders = findShaderFiles();
        std::string selectedShader = fetchWantedFile(shaders);
        
        if (selectedShader.empty()) {
            std::cerr << "No .comp files found in shaders directory :/" << std::endl;
            glDeleteTextures(1, &tex);
            glfwDestroyWindow(win);
            glfwTerminate();
            return 0;
        }

        progCompute = buildComputeProgramFromFile(selectedShader.c_str());
        progBlit    = buildProgramFromFiles("shaders/fullscreen.vert", "shaders/blit.frag");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Shader build error: %s\n", e.what());
        glDeleteTextures(1, &tex);
        glfwDestroyWindow(win);
        glfwTerminate();
        return 1;
    }

    GLint uTimeLoc = glGetUniformLocation(progCompute, "uTime");
    const GLint uTexLoc  = glGetUniformLocation(progBlit, "uTex");

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);

    static bool menuKeyPressed = false;

    std::cout << "\n=== CONTROLS ===\n";
    std::cout << "M - Switch shader (opens menu in console)\n";
    std::cout << "ESC - Exit\n\n";
    std::cout << "Press M at any time to change shader\n\n";
    std::cout.flush();

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
        
        // Generate image with compute shader
        glUseProgram(progCompute);
        if (uTimeLoc >= 0) glUniform1f(uTimeLoc, t);

        glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

        const GLuint gx = (GLuint)((texW + 15) / 16);
        const GLuint gy = (GLuint)((texH + 15) / 16);
        glDispatchCompute(gx, gy, 1);
        
        // Wait for compute to finish
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        glClear(GL_COLOR_BUFFER_BIT);

        // Display with vertex + fragment shader
        glUseProgram(progBlit);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        if (uTexLoc >= 0) glUniform1i(uTexLoc, 0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        bool reloadShader = false;
        if (glfwGetKey(win, GLFW_KEY_M) == GLFW_PRESS && !menuKeyPressed) {
            menuKeyPressed = true;
            reloadShader = true;
        }
        if (glfwGetKey(win, GLFW_KEY_M) == GLFW_RELEASE) {
            menuKeyPressed = false;
        }

        if (reloadShader) {
            // Freeze the current frame
            glfwSwapBuffers(win);
            
            // Clear the console and show menu prominently
            std::cout << "\n\n";
            std::cout << "============================================\n";
            std::cout << "  PAUSED - CLICK THIS CONSOLE WINDOW NOW\n";
            std::cout << "============================================\n\n";
            
            // Flush to make sure it appears immediately
            std::cout.flush();
            
            vector<std::string> shaders = findShaderFiles();
            std::string newShader = fetchWantedFile(shaders);
            
            if (!newShader.empty()) {
                try {
                    glDeleteProgram(progCompute);
                    progCompute = buildComputeProgramFromFile(newShader.c_str());
                    uTimeLoc = glGetUniformLocation(progCompute, "uTime");
                    
                    std::cout << "\nShader loaded successfully!\n";
                    std::cout << "Press SPACE in the graphics window to resume...\n\n";
                    std::cout.flush();
                } catch (const std::exception& e) {
                    std::fprintf(stderr, "Reload failed: %s\n", e.what());
                    std::cout << "Press SPACE in the graphics window to resume...\n\n";
                    std::cout.flush();
                }
            } else {
                // Terminate the program
                std::cout << "\nExiting program...\n";
                glfwSetWindowShouldClose(win, GLFW_TRUE);
                continue;  // Skip the SPACE wait and go to cleanup
            }
            
            // Wait for SPACE to resume
            bool spacePressed = false;
            while (!spacePressed && !glfwWindowShouldClose(win)) {
                glfwPollEvents();
                
                if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) {
                    spacePressed = true;
                }
                
                // Sleep a bit to not burn CPU
                glfwWaitEventsTimeout(0.1);
            }
            
            // Clear the space key state
            while (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) {
                glfwPollEvents();
            }
            
            std::cout << "Resuming...\n\n";
        }

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
