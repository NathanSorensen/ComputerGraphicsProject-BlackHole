#include "RayTracer.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

RayTracer::RayTracer(const std::string& computePath,
    const std::string& quadVertPath,
    const std::string& quadFragPath) {
    // 1. Compile and link compute shader
    GLuint comp = compileShader(GL_COMPUTE_SHADER, readFile(computePath), computePath);
    computeProgram = glCreateProgram();
    glAttachShader(computeProgram, comp);
    glLinkProgram(computeProgram);
    glDeleteShader(comp);

    // 2. Compile and link fullscreen quad shaders
    GLuint vert = compileShader(GL_VERTEX_SHADER, readFile(quadVertPath), quadVertPath);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, readFile(quadFragPath), quadFragPath);
    quadProgram = glCreateProgram();
    glAttachShader(quadProgram, vert);
    glAttachShader(quadProgram, frag);
    glLinkProgram(quadProgram);
    glDeleteShader(vert);
    glDeleteShader(frag);

    // 3. Create fullscreen quad geometry (pos.xy, uv.xy)
    const float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVao);
    glGenBuffers(1, &quadVbo);

    glBindVertexArray(quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // 4. Create output storage texture
    glGenTextures(1, &outputTexture);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

RayTracer::~RayTracer() {
    if (outputTexture) glDeleteTextures(1, &outputTexture);
    if (quadVbo)       glDeleteBuffers(1, &quadVbo);
    if (quadVao)       glDeleteVertexArrays(1, &quadVao);
    if (computeProgram) glDeleteProgram(computeProgram);
    if (quadProgram)    glDeleteProgram(quadProgram);
}

void RayTracer::ensureTextureSize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width == texWidth && height == texHeight) return;

    texWidth = width;
    texHeight = height;

    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, texWidth, texHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RayTracer::render(int width, int height,
    const Camera& camera,
    const std::vector<BlackHole>& blackHoles,
    bool showDisc,
    bool showGrid,
    float time) {
    if (width <= 0 || height <= 0) return;
    ensureTextureSize(width, height);

    // 1. Compute camera orthonormal basis matching glm::lookAt
    glm::vec3 camPos = camera.getPosition();
    glm::vec3 forward = camera.getForward();
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::cross(right, forward);
    float tanHalfFov = std::tan(glm::radians(camera.fovDegrees) * 0.5f);
    float aspect = static_cast<float>(width) / static_cast<float>(height);

    // 2. Dispatch geodesic.comp on the GPU
    glUseProgram(computeProgram);
    glUniform3fv(glGetUniformLocation(computeProgram, "uCamPos"), 1, glm::value_ptr(camPos));
    glUniform3fv(glGetUniformLocation(computeProgram, "uCamForward"), 1, glm::value_ptr(forward));
    glUniform3fv(glGetUniformLocation(computeProgram, "uCamRight"), 1, glm::value_ptr(right));
    glUniform3fv(glGetUniformLocation(computeProgram, "uCamUp"), 1, glm::value_ptr(up));
    glUniform1f(glGetUniformLocation(computeProgram, "uTanHalfFov"), tanHalfFov);
    glUniform1f(glGetUniformLocation(computeProgram, "uAspect"), aspect);

    int count = static_cast<int>(glm::min(blackHoles.size(), size_t(2)));
    glUniform1i(glGetUniformLocation(computeProgram, "uBhCount"), count);

    glm::vec4 bhData[2] = { glm::vec4(0.0f), glm::vec4(0.0f) };
    for (int i = 0; i < count; ++i) {
        bhData[i] = glm::vec4(blackHoles[i].pos, blackHoles[i].rs);
    }
    glUniform4fv(glGetUniformLocation(computeProgram, "uBlackHoles"), 2, glm::value_ptr(bhData[0]));
    glUniform1i(glGetUniformLocation(computeProgram, "uShowDisc"), showDisc ? 1 : 0);
    glUniform1i(glGetUniformLocation(computeProgram, "uShowGrid"), showGrid ? 1 : 0);
    glUniform1f(glGetUniformLocation(computeProgram, "uTime"), time);

    glBindImageTexture(0, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // 3. Draw fullscreen quad displaying the raytraced texture (behind 3D grid)
    glDisable(GL_DEPTH_TEST);
    glUseProgram(quadProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glUniform1i(glGetUniformLocation(quadProgram, "uScreenTex"), 0);

    glBindVertexArray(quadVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

std::string RayTracer::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[RayTracer Error] Could not open file: " << path << "\n";
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint RayTracer::compileShader(GLenum type, const std::string& source, const std::string& label) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "[RayTracer Compile Error] (" << label << "): " << infoLog << "\n";
    }
    return shader;
}
