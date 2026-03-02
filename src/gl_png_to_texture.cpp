#include "gl_png_to_texture.h"

GLuint loadTextureFromPNG(const char* path)
{
    std::vector<unsigned char> pixels;
    unsigned width = 0, height = 0;

    unsigned error = lodepng::decode(pixels, width, height, path);
    if (error)
        throw std::runtime_error(lodepng_error_text(error));

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
        (GLsizei)width, (GLsizei)height,
        0, GL_RGBA, GL_UNSIGNED_BYTE,
        pixels.data());

    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}