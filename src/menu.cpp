#include "menu.h"
#include "gl_shader.h"

#include <iostream>
#include <algorithm>
#include <cstdio>

using namespace std;

vector<std::string> findShaderFiles() {
    vector<std::string> shaderFiles;
    std::filesystem::path shadersDir = "shaders";

    // Check if folder exists
    if (!std::filesystem::exists(shadersDir)) {
        std::cout << "Error: shaders directory not found!\n";
        return shaderFiles;
    }

    // Collect all .comp files
    for (const auto& entry : std::filesystem::directory_iterator(shadersDir)) {
        const std::string filename = entry.path().filename().string();

        if (entry.is_regular_file() && entry.path().extension() == ".comp" && filename.find("_update") == std::string::npos) {
            shaderFiles.push_back(entry.path().string());
        }
    }

    // Sort alphabetically
    std::sort(shaderFiles.begin(), shaderFiles.end());
    return shaderFiles;
}

std::string fetchWantedFile(vector<std::string> files) {
    if (files.empty()) {
            std::cout << "No .comp files found in shaders directory :/\n";
        return "";
    }

    // Show numbered list of available shaders
    std::cout << "\n=== Shader Menu ===\n";
    for (size_t i = 0; i < files.size(); i++) {
        std::filesystem::path p(files[i]);
        std::cout << "[" << i + 1 << "] " << p.filename().string() << "\n";
    }
    std::cout << "[0] Exit\n";
    std::cout << "\nSelect shader: ";

    int choice;
    std::cin >> choice;

    if (choice == 0) {
        std::cout << "Exiting...\n";
        return "";
    }

    if (choice < 1 || choice > static_cast<int>(files.size())) {
        std::cout << "Invalid choice! Using default shader." << "\n";
        return files[0];
    }

    std::cout << "Loading: " << files[choice - 1] << "\n\n";
    return files[choice - 1];
}

void printControls() {
    std::cout << "\n=== CONTROLS ===\n";
    std::cout << "M - Switch shader (opens menu in console)\n";
    std::cout << "ESC - Exit\n\n";
    std::cout << "Press M at any time to change shader\n\n";
    std::cout.flush();
}

// Freeze the window, show the shader menu, and reload on selection
static bool handleShaderSwitch(GLFWwindow* win, GLuint& progCompute, GLint& uTimeLoc, PingPong& pp) {
    glfwSwapBuffers(win);

    std::cout << "\n\n";
    std::cout << "============================================\n";
    std::cout << "  PAUSED - CLICK THIS CONSOLE WINDOW NOW\n";
    std::cout << "============================================\n\n";
    std::cout.flush();

    vector<std::string> shaders = findShaderFiles();
    std::string newShader = fetchWantedFile(shaders);

    if (newShader.empty()) {
        std::cout << "\nExiting program...\n";
        glfwSetWindowShouldClose(win, GLFW_TRUE);
        return false;
    }

    try {
        glDeleteProgram(progCompute);
        pp.destroy();

        progCompute = buildComputeProgramFromFile(newShader.c_str());
        uTimeLoc    = glGetUniformLocation(progCompute, "uTime");
        pp.load(newShader);

        std::cout << "\nShader loaded successfully!\n";
        std::cout << "Press SPACE in the graphics window to resume...\n\n";
        std::cout.flush();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Reload failed: %s\n", e.what());
        std::cout << "Press SPACE in the graphics window to resume...\n\n";
        std::cout.flush();
    }

    // Wait for SPACE before resuming
    bool spacePressed = false;
    while (!spacePressed && !glfwWindowShouldClose(win)) {
        glfwPollEvents();
        if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS)
            spacePressed = true;
        glfwWaitEventsTimeout(0.1);
    }
    while (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS)
        glfwPollEvents();

    std::cout << "Resuming...\n\n";
    return true;
}

bool pollMenuKey(GLFWwindow* win, GLuint& progCompute, GLint& uTimeLoc, PingPong& pp) {
    // Track key state to avoid repeated triggers on hold
    static bool menuKeyPressed = false;

    if (glfwGetKey(win, GLFW_KEY_M) == GLFW_PRESS && !menuKeyPressed) {
        menuKeyPressed = true;
        return handleShaderSwitch(win, progCompute, uTimeLoc, pp);
    }
    if (glfwGetKey(win, GLFW_KEY_M) == GLFW_RELEASE)
        menuKeyPressed = false;

    return true;
}