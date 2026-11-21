#pragma once

#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "Camera.hpp"
#include "Simulation.hpp"

class RayTracer {
public:
    RayTracer(const std::string& computePath,
        const std::string& quadVertPath,
        const std::string& quadFragPath);
    ~RayTracer();

    void render(int width, int height,
        const Camera& camera,
        const std::vector<BlackHole>& blackHoles,
        bool showDisc,
        bool showGrid,
        float time);

private:
    GLuint computeProgram = 0;
    GLuint quadProgram = 0;
    GLuint outputTexture = 0;
    GLuint quadVao = 0;
    GLuint quadVbo = 0;

    int texWidth = 0;
    int texHeight = 0;

    void ensureTextureSize(int width, int height);
    static std::string readFile(const std::string& path);
    static GLuint compileShader(GLenum type, const std::string& source, const std::string& label);
};
