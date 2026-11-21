#pragma once

#include <array>
#include <chrono>
#include <vector>

#include <GL/glew.h>
#include <nanogui/nanogui.h>

#include "Renderer.hpp"
#include "Camera.hpp"
#include "Simulation.hpp"
#include "GuiPanel.hpp"
#include "RayTracer.hpp"

class BlackHoleVisualSim : public nanogui::Screen {
public:
    explicit BlackHoleVisualSim(bool is2DMode);
    ~BlackHoleVisualSim() override;

    void drawContents() override;

    bool mouseButtonEvent(const Eigen::Vector2i& p, int button, bool down, int modifiers) override;
    bool mouseMotionEvent(const Eigen::Vector2i& p, const Eigen::Vector2i& rel, int button, int modifiers) override;
    bool scrollEvent(const Eigen::Vector2i& p, const Eigen::Vector2f& rel) override;
    bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

private:
    bool mIs2DMode = true;
    SimParams mParams;
    Camera mCamera;
    Renderer* mRenderer = nullptr;
    RayTracer* mRayTracer = nullptr;
    float mSimTime = 0.0f;
    nanogui::Window* mControlWindow = nullptr;

    std::vector<BlackHole> mBlackHoles;
    std::vector<Ray> mRays;
    float mBinaryAngle = 1.5707963f; // Starts aligned vertically along Y axis

    bool mDragging = false;
    std::array<bool, 512> mKeys{};
    std::chrono::steady_clock::time_point mLastFrameTime;

    float computeDeltaTime();
    void updateCameraMovement(float dt);
    void syncBlackHoles(float dt);

    // 2D Mode
    void spawnParallelRays(float startX = -12.0f);
    void update2D(float dt);
    void render2D();

    // 3D Mode
    void update3D(float dt);
    void render3D();
};
