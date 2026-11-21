#pragma once

#include <vector>
#include <glm/glm.hpp>

struct BlackHole {
    glm::vec3 pos{ 0.0f, 0.0f, 0.0f };
    float rs = 1.5f;
    glm::vec4 ringColor{ 1.0f, 0.55f, 0.15f, 0.9f };
};

struct SimParams {
    // Simulation & Ray Controls
    float timeScale = 6.0f;
    float cameraSpeed = 6.0f;
    int rayCount = 30;
    float raySpreadY = 7.0f;
    float rayAngleDeg = 0.0f;
    bool paused = false;
    bool resetRequested = false;

    // Black Hole 1
    float bh1Rs = 1.0f;
    float bh1X = 0.0f;
    float bh1Y = -0.0f;

    // Black Hole 2
    bool enableSecondBH = false;
    float bh2Rs = 1.0f;
    float bh2X = 0.0f;
    float bh2Y = -10.0f;

    // Binary Orbit Mode
    bool binaryOrbit = false;
    float binarySeparation = 10.0f;
    float binarySpeed = 0.2f; // Radians per second

    // 3D Mode Toggles
    bool enableRayTracing = true;
    bool showAccretionDisc = false;
    bool showSpacetimeGrid = false;
};

struct RayState {
    glm::vec3 pos;
    glm::vec3 vel;
};

// Schwarzschild null geodesic acceleration from a single black hole (c = 1)
inline glm::vec3 computeSingleGeodesicAccel(const glm::vec3& pos, const glm::vec3& vel, const BlackHole& bh) {
    glm::vec3 relPos = pos - bh.pos;
    float r = glm::length(relPos);
    if (r <= 1e-4f) return glm::vec3(0.0f);

    glm::vec3 L = glm::cross(relPos, vel);
    float L2 = glm::dot(L, L);
    float r5 = r * r * r * r * r;

    return -1.5f * bh.rs * (L2 / r5) * relPos;
}

// Total geodesic acceleration summed across all active black holes
inline glm::vec3 computeTotalGeodesicAccel(const glm::vec3& pos, const glm::vec3& vel,
    const std::vector<BlackHole>& blackHoles) {
    glm::vec3 totalAccel(0.0f);
    for (const auto& bh : blackHoles) {
        totalAccel += computeSingleGeodesicAccel(pos, vel, bh);
    }
    return totalAccel;
}

// 4th-Order Runge-Kutta step across all active black holes
inline RayState rk4Step(const RayState& state, float dt, const std::vector<BlackHole>& blackHoles) {
    glm::vec3 v1 = state.vel;
    glm::vec3 a1 = computeTotalGeodesicAccel(state.pos, v1, blackHoles);

    glm::vec3 p2 = state.pos + v1 * (0.5f * dt);
    glm::vec3 v2 = state.vel + a1 * (0.5f * dt);
    glm::vec3 a2 = computeTotalGeodesicAccel(p2, v2, blackHoles);

    glm::vec3 p3 = state.pos + v2 * (0.5f * dt);
    glm::vec3 v3 = state.vel + a2 * (0.5f * dt);
    glm::vec3 a3 = computeTotalGeodesicAccel(p3, v3, blackHoles);

    glm::vec3 p4 = state.pos + v3 * dt;
    glm::vec3 v4 = state.vel + a3 * dt;
    glm::vec3 a4 = computeTotalGeodesicAccel(p4, v4, blackHoles);

    RayState next;
    next.pos = state.pos + (dt / 6.0f) * (v1 + 2.0f * v2 + 2.0f * v3 + v4);
    next.vel = state.vel + (dt / 6.0f) * (a1 + 2.0f * a2 + 2.0f * a3 + a4);
    return next;
}

struct Ray {
    glm::vec3 pos{ 0.0f };
    glm::vec3 vel{ 0.0f };
    std::vector<glm::vec3> trail;
    bool absorbed = false;
    size_t maxTrailPoints = 350;

    Ray(const glm::vec3& startPos, const glm::vec3& initialVel)
        : pos(startPos), vel(initialVel) {
        trail.reserve(maxTrailPoints);
        trail.push_back(pos);
    }

    void step(float dt, const std::vector<BlackHole>& blackHoles) {
        if (absorbed) return;

        const int subSteps = 4;
        float subDt = dt / static_cast<float>(subSteps);

        for (int i = 0; i < subSteps; ++i) {
            for (const auto& bh : blackHoles) {
                if (glm::length(pos - bh.pos) <= bh.rs) {
                    absorbed = true;
                    return;
                }
            }

            RayState next = rk4Step({ pos, vel }, subDt, blackHoles);
            pos = next.pos;
            vel = next.vel;
        }

        trail.push_back(pos);
        if (trail.size() > maxTrailPoints) {
            trail.erase(trail.begin());
        }
    }
};
