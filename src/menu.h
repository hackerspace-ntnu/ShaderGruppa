#pragma once
#include <string>
#include <vector>
#include <filesystem>

using namespace std;

// Find all .comp files in the shaders folder
vector<std::string> findShaderFiles();

// Show menu and fetch the user's choice
std::string fetchWantedFile(vector<std::string> files);