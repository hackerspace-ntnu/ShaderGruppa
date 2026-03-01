#pragma once
#include <string>
#include <vector>
#include <filesystem>

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "pingpong.h"

using namespace std;

// Find all .comp files in the shaders folder
vector<std::string> findShaderFiles();

// Show menu and return the user's chosen shader path
std::string fetchWantedFile(vector<std::string> files);

// Print controls to the console
void printControls();

// Poll the M key and handle shader switching if pressed.
// Returns false if the render loop should call continue.
bool pollMenuKey(GLFWwindow* win, GLuint& progCompute, GLint& uTimeLoc, PingPong& pp);