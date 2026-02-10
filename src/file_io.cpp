#include "file_io.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

std::string readTextFile(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error(std::string("Failed to open file: ") + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}