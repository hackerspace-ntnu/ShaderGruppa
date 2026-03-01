#pragma once
#include <string>
#include <glad/glad.h>

static const int MAX_PARTICLES = 4096;

struct PingPong {
    GLuint ssbo[2] = {0, 0};
    GLuint progUpdate = 0;
    int ping = 0, pong = 1;

    // Laster update-shader (X_render.comp -> X_update.comp) og initialiserer SSBOs
    void load(const std::string& renderShaderPath);

    // Kjører update-pass (hvis update-shader finnes) og swapper ping/pong
    void update(float t, float dt);

    // Binder ssbo[ping] til binding 1 (les) og ssbo[pong] til binding 2 (skriv)
    void bindBuffers() const;

    // Initialiserer SSBOs på nytt med tilfeldige posisjoner
    void reset();

    // Frigjør plass
    void destroy();
};