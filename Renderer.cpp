#include "Renderer.hpp"
#include <cmath>

Renderer::Renderer(const std::string& vertPath, const std::string& fragPath)
    : shader(vertPath, fragPath) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Attribute 0: vec3 position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, pos));
    glEnableVertexAttribArray(0);

    // Attribute 1: vec4 color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
}

Renderer::~Renderer() {
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (vao != 0) glDeleteVertexArrays(1, &vao);
}

void Renderer::beginFrame(const glm::mat4& view, const glm::mat4& projection) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader.use();
    shader.setMat4("uView", view);
    shader.setMat4("uProjection", projection);
}

void Renderer::submitBatch(const std::vector<Vertex3D>& vertices, GLenum drawMode) {
    if (vertices.empty()) return;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3D), vertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(drawMode, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
}

void Renderer::drawCircle(const glm::vec3& center, float radius,
    const glm::vec4& fillColor, const glm::vec4& outlineColor,
    int segments) {
    constexpr float PI = 3.14159265359f;

    // 1. Filled circle (Triangle Fan)
    std::vector<Vertex3D> fillVerts;
    fillVerts.reserve(segments + 2);
    fillVerts.push_back({ center, fillColor });

    for (int i = 0; i <= segments; ++i) {
        float theta = (2.0f * PI * i) / segments;
        glm::vec3 offset(std::cos(theta) * radius, std::sin(theta) * radius, 0.0f);
        fillVerts.push_back({ center + offset, fillColor });
    }
    submitBatch(fillVerts, GL_TRIANGLE_FAN);

    // 2. Event horizon outline ring (Line Loop, offset slightly on Z to avoid z-fighting)
    std::vector<Vertex3D> ringVerts;
    ringVerts.reserve(segments);
    for (int i = 0; i < segments; ++i) {
        float theta = (2.0f * PI * i) / segments;
        glm::vec3 offset(std::cos(theta) * radius, std::sin(theta) * radius, 0.005f);
        ringVerts.push_back({ center + offset, outlineColor });
    }
    submitBatch(ringVerts, GL_LINE_LOOP);
}

void Renderer::drawFadingTrail(const std::vector<glm::vec3>& points, const glm::vec3& rgbColor) {
    if (points.size() < 2) return;

    std::vector<Vertex3D> trailVerts;
    trailVerts.reserve(points.size());

    for (size_t i = 0; i < points.size(); ++i) {
        // Alpha increases from 0.0 at tail to 1.0 at ray head
        float alpha = static_cast<float>(i + 1) / static_cast<float>(points.size());
        trailVerts.push_back({ points[i], glm::vec4(rgbColor, alpha) });
    }
    submitBatch(trailVerts, GL_LINE_STRIP);
}

void Renderer::drawPoint(const glm::vec3& pos, const glm::vec4& color, float size) {
    glPointSize(size);
    submitBatch({ {pos, color} }, GL_POINTS);
}

void Renderer::drawSpacetimeGrid(const std::vector<BlackHole>& blackHoles,
    float halfExtent, int divisions, float depthScale) {
    constexpr float PI = 3.14159265359f;
    glm::vec4 flatColor(0.22f, 0.36f, 0.58f, 0.45f);
    glm::vec4 deepColor(1.00f, 0.52f, 0.15f, 0.90f);

    // Evaluates the Flamm's paraboloid embedding height & color at any (x, z)
    auto evalPoint = [&](float x, float z) -> Vertex3D {
        float y = 0.0f;
        for (const auto& bh : blackHoles) {
            float dx = x - bh.pos.x;
            float dz = z - bh.pos.z;
            float r = std::sqrt(dx * dx + dz * dz);

            float hLocal = 2.0f * std::sqrt(bh.rs * glm::max(r - bh.rs, 0.0f));
            float hRef = 2.0f * std::sqrt(bh.rs * glm::max(halfExtent - bh.rs, 0.0f));
            y += depthScale * glm::min(hLocal - hRef, 0.0f);
        }
        float depthT = glm::clamp(-y / 5.5f, 0.0f, 1.0f);
        return { glm::vec3(x, y, z), glm::mix(flatColor, deepColor, depthT) };
        };

    auto isInsideAnyHorizon = [&](float x, float z) -> bool {
        for (const auto& bh : blackHoles) {
            float dx = x - bh.pos.x;
            float dz = z - bh.pos.z;
            if (dx * dx + dz * dz < bh.rs * bh.rs) return true;
        }
        return false;
        };

    // Finds the exact t in [0, 1] where segment A -> B crosses a black hole's Rs circle
    auto findHorizonHitT = [&](glm::vec2 a, glm::vec2 b, float& outT) -> bool {
        glm::vec2 d = b - a;
        float dd = glm::dot(d, d);
        if (dd < 1e-8f) return false;

        bool hit = false;
        float bestT = 2.0f;

        for (const auto& bh : blackHoles) {
            glm::vec2 c(bh.pos.x, bh.pos.z);
            glm::vec2 f = a - c;
            float bCoef = 2.0f * glm::dot(f, d);
            float cCoef = glm::dot(f, f) - bh.rs * bh.rs;
            float disc = bCoef * bCoef - 4.0f * dd * cCoef;

            if (disc >= 0.0f) {
                float sqrtDisc = std::sqrt(disc);
                float t1 = (-bCoef - sqrtDisc) / (2.0f * dd);
                float t2 = (-bCoef + sqrtDisc) / (2.0f * dd);

                if (t1 >= 0.0f && t1 <= 1.0f && t1 < bestT) { bestT = t1; hit = true; }
                if (t2 >= 0.0f && t2 <= 1.0f && t2 < bestT) { bestT = t2; hit = true; }
            }
        }
        if (hit) outT = bestT;
        return hit;
        };

    struct GridNode {
        Vertex3D vertex;
        bool inside = false;
    };

    int stride = divisions + 1;
    std::vector<GridNode> lattice(stride * stride);

    for (int row = 0; row < stride; ++row) {
        float z = glm::mix(-halfExtent, halfExtent, static_cast<float>(row) / divisions);
        for (int col = 0; col < stride; ++col) {
            float x = glm::mix(-halfExtent, halfExtent, static_cast<float>(col) / divisions);
            lattice[row * stride + col] = { evalPoint(x, z), isInsideAnyHorizon(x, z) };
        }
    }

    std::vector<Vertex3D> lineVerts;
    lineVerts.reserve(divisions * stride * 4);

    auto addClippedSegment = [&](const GridNode& nA, const GridNode& nB) {
        if (nA.inside && nB.inside) return; // Completely inside event horizon

        if (!nA.inside && !nB.inside) {
            // Both endpoints outside (still check if a thin horizon sits between them)
            float tHit = 0.0f;
            glm::vec2 a2(nA.vertex.pos.x, nA.vertex.pos.z);
            glm::vec2 b2(nB.vertex.pos.x, nB.vertex.pos.z);
            if (!findHorizonHitT(a2, b2, tHit)) {
                lineVerts.push_back(nA.vertex);
                lineVerts.push_back(nB.vertex);
            }
            return;
        }

        // One endpoint is outside and one is inside -> clip directly onto the r = Rs circle
        const GridNode& outNode = nA.inside ? nB : nA;
        const GridNode& inNode = nA.inside ? nA : nB;

        glm::vec2 a2(outNode.vertex.pos.x, outNode.vertex.pos.z);
        glm::vec2 b2(inNode.vertex.pos.x, inNode.vertex.pos.z);

        float tHit = 0.0f;
        if (findHorizonHitT(a2, b2, tHit)) {
            glm::vec2 hitXZ = glm::mix(a2, b2, tHit);
            lineVerts.push_back(outNode.vertex);
            lineVerts.push_back(evalPoint(hitXZ.x, hitXZ.y));
        }
        };

    for (int row = 0; row < stride; ++row) {
        for (int col = 0; col < stride; ++col) {
            int idx = row * stride + col;
            if (col < divisions) addClippedSegment(lattice[idx], lattice[idx + 1]);
            if (row < divisions) addClippedSegment(lattice[idx], lattice[idx + stride]);
        }
    }

    submitBatch(lineVerts, GL_LINES);

    // 3. Draw a crisp circular throat rim around each black hole at r = Rs
    //const int ringSegments = 72;
    //for (const auto& bh : blackHoles) {
    //    std::vector<Vertex3D> rimVerts;
    //    rimVerts.reserve(ringSegments);
    //    for (int i = 0; i < ringSegments; ++i) {
    //        float theta = (2.0f * PI * i) / ringSegments;
    //        float rx = bh.pos.x + std::cos(theta) * bh.rs;
    //        float rz = bh.pos.z + std::sin(theta) * bh.rs;
    //        Vertex3D v = evalPoint(rx, rz);
    //        v.color = deepColor;
    //        rimVerts.push_back(v);
    //    }
    //    submitBatch(rimVerts, GL_LINE_LOOP);
    //}
}


void Renderer::drawSphere(const glm::vec3& center, float radius, const glm::vec4& color,
    int stacks, int slices) {
    constexpr float PI = 3.14159265359f;
    std::vector<Vertex3D> verts;
    verts.reserve(stacks * slices * 6);

    auto getPoint = [&](int stack, int slice) -> glm::vec3 {
        float phi = PI * static_cast<float>(stack) / static_cast<float>(stacks);
        float theta = 2.0f * PI * static_cast<float>(slice) / static_cast<float>(slices);
        return center + radius * glm::vec3(
            std::sin(phi) * std::cos(theta),
            std::cos(phi),
            std::sin(phi) * std::sin(theta)
        );
        };

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            glm::vec3 p00 = getPoint(i, j);
            glm::vec3 p10 = getPoint(i + 1, j);
            glm::vec3 p11 = getPoint(i + 1, j + 1);
            glm::vec3 p01 = getPoint(i, j + 1);

            verts.push_back({ p00, color });
            verts.push_back({ p10, color });
            verts.push_back({ p11, color });

            verts.push_back({ p00, color });
            verts.push_back({ p11, color });
            verts.push_back({ p01, color });
        }
    }

    submitBatch(verts, GL_TRIANGLES);
}
