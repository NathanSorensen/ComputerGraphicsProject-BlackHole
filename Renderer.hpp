#pragma once

#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "Shader.hpp"
#include "Simulation.hpp"

struct Vertex3D {
    glm::vec3 pos;
    glm::vec4 color;
};

class Renderer {
public:
    Renderer(const std::string& vertPath, const std::string& fragPath);
    ~Renderer();

    void beginFrame(const glm::mat4& view, const glm::mat4& projection);

    // 2D Primitives (XY plane)
    void drawCircle(const glm::vec3& center, float radius,
        const glm::vec4& fillColor, const glm::vec4& outlineColor,
        int segments = 64);
    void drawFadingTrail(const std::vector<glm::vec3>& points, const glm::vec3& rgbColor);
    void drawPoint(const glm::vec3& pos, const glm::vec4& color, float size = 5.0f);

    // 3D Primitives (XZ spacetime grid & event horizon sphere)
    void drawSpacetimeGrid(const std::vector<BlackHole>& blackHoles,
        float halfExtent = 18.0f, int divisions = 96, float depthScale = 0.55f);
    void drawSphere(const glm::vec3& center, float radius, const glm::vec4& color,
        int stacks = 24, int slices = 32);

private:
    Shader shader;
    GLuint vao = 0;
    GLuint vbo = 0;

    void submitBatch(const std::vector<Vertex3D>& vertices, GLenum drawMode);
};
