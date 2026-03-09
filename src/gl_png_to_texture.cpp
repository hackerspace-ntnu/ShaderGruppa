#include "gl_png_to_texture.h"
#include "lodepng.h"
#include <vector>
#include <stdexcept>

GLuint loadTextureFromPNG(const char* path, unsigned targetWidth, unsigned targetHeight)
{
    std::vector<unsigned char> pixels;
    unsigned width = 0, height = 0;

    unsigned error = lodepng::decode(pixels, width, height, path);
    if (error)
        throw std::runtime_error(lodepng_error_text(error));

    // Flip image vertically
    unsigned bytesPerPixel = 4; // RGBA
    for (unsigned y = 0; y < height / 2; y++) {
        for (unsigned x = 0; x < width; x++) {
            unsigned idx1 = (y * width + x) * bytesPerPixel;
            unsigned idx2 = ((height - 1 - y) * width + x) * bytesPerPixel;
            for (unsigned i = 0; i < bytesPerPixel; i++) {
                std::swap(pixels[idx1 + i], pixels[idx2 + i]);
            }
        }
    }

    // Resize if needed
    if (width != targetWidth || height != targetHeight) {
        std::vector<unsigned char> resized(targetWidth * targetHeight * bytesPerPixel);
        for (unsigned y = 0; y < targetHeight; y++) {
            for (unsigned x = 0; x < targetWidth; x++) {
                unsigned srcX = (x * width) / targetWidth;
                unsigned srcY = (y * height) / targetHeight;
                unsigned srcIdx = (srcY * width + srcX) * bytesPerPixel;
                unsigned dstIdx = (y * targetWidth + x) * bytesPerPixel;
                for (unsigned i = 0; i < bytesPerPixel; i++) {
                    resized[dstIdx + i] = pixels[srcIdx + i];
                }
            }
        }
        pixels = std::move(resized);
        width = targetWidth;
        height = targetHeight;
    }

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