#include "menu.h"
#include <iostream>
#include <algorithm>

using namespace std;

vector<std::string> findShaderFiles() {
    vector<std::string> shaderFiles;
    std::filesystem::path shadersDir = "shaders";
    
    // Check if folder exists
    if (!std::filesystem::exists(shadersDir)) {
        std::cout << "Error: shaders directory not found!" << std::endl;
        return shaderFiles;
    }
    
    // Iterate through all the files in the shaders folder 
    for (const auto& entry : std::filesystem::directory_iterator(shadersDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".comp") {
            shaderFiles.push_back(entry.path().string());
        }
    }
    
    // Sort alfabetically
    std::sort(shaderFiles.begin(), shaderFiles.end());
    
    return shaderFiles;
}

std::string fetchWantedFile(vector<std::string> files) {
    if (files.empty()) {
        std::cout << "No .comp files found in shaders directory :/" << std::endl;
        return "";
    }
    
    // Show menu
    std::cout << "\n=== Shader Menu ===\n";
    for (size_t i = 0; i < files.size(); i++) {

        // Fetch filename from path
        std::filesystem::path p(files[i]);
        std::cout << "[" << i + 1 << "] " << p.filename().string() << std::endl;
    }
    std::cout << "[0] Exit\n";
    std::cout << "\nSelect shader: ";
    
    // Read input from user
    int choice;
    std::cin >> choice;
    
    // Validate input
    if (choice == 0) {
        std::cout << "Exiting...\n";
        return "";
    }
    
    if (choice < 1 || choice > static_cast<int>(files.size())) {
        std::cout << "Invalid choice! Using default shader." << std::endl;
        return files[0];
    }
    
    std::cout << "Loading: " << files[choice - 1] << "\n\n";
    return files[choice - 1];
}