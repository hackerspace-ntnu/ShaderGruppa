#include "pingpong.h"
#include "gl_shader.h"

#include <algorithm>
#include <fstream>
#include <random>
#include <vector>
#include <cstdio>

static void initSSBOs(GLuint ssbo[2]) {
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::vector<float> data(MAX_PARTICLES * 2);
    for (auto& v : data) v = dist(rng);

    glGenBuffers(2, ssbo);
    for (int i = 0; i < 2; i++) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[i]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_COPY);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

static std::string findUpdateShader(const std::string& renderPath) {
    const std::string suffix = "_render.comp";
    if (renderPath.size() > suffix.size() &&
        renderPath.compare(renderPath.size() - suffix.size(), suffix.size(), suffix) == 0) {
        std::string updatePath = renderPath.substr(0, renderPath.size() - suffix.size()) + "_update.comp";
        if (std::ifstream(updatePath).good())
            return updatePath;
    }
    return {};
}

void PingPong::load(const std::string& renderShaderPath) {
    initSSBOs(ssbo);

    std::string updatePath = findUpdateShader(renderShaderPath);
    if (!updatePath.empty()) {
        try {
            progUpdate = buildComputeProgramFromFile(updatePath.c_str());
            std::printf("Ping-pong update shader loaded: %s\n", updatePath.c_str());
        } catch (const std::exception& e) {
            std::fprintf(stderr, "Update shader error: %s\n", e.what());
        }
    }
}

void PingPong::update(float t, float dt) {
    if (!progUpdate) return;

    glUseProgram(progUpdate);
    glUniform1f(glGetUniformLocation(progUpdate, "uTime"), t);
    glUniform1f(glGetUniformLocation(progUpdate, "dt"),    dt);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[ping]); // read
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo[pong]); // write

    glDispatchCompute((MAX_PARTICLES + 255) / 256, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    std::swap(ping, pong);
}

void PingPong::bindBuffers() const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[ping]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo[pong]);
}

void PingPong::reset() {
    ping = 0; pong = 1;
    initSSBOs(ssbo);
}

void PingPong::destroy() {
    glDeleteBuffers(2, ssbo);
    glDeleteProgram(progUpdate);
    ssbo[0] = ssbo[1] = progUpdate = 0;
}