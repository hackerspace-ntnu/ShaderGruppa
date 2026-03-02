#pragma once
#include <vector>
#include <stdexcept>
#include <string>

#include <glad/glad.h>
#include "lodepng.h"

GLuint loadImageAsTextureRGBA8(const char* path);