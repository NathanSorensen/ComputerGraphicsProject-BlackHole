#include "BlackHoleVisualSim.hpp"
#include <cmath>
#include <stdexcept>

BlackHoleVisualSim::BlackHoleVisualSim(bool is2DMode)
    : nanogui::Screen(nanogui::Vector2i(1000, 750), "Black Hole Simulation"),
    mIs2DMode(is2DMode) {
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    setBackground(nanogui::Color(0.04f, 0.04f, 0.07f, 1.0f));
    mControlWindow = GuiPanel::build(this, mParams, is2DMode);
    performLayout();

    mRenderer = new Renderer("shaders/basic.vert", "shaders/basic.frag");
    syncBlackHoles(0.0f);

    if (mIs2DMode) {
        spawnParallelRays();
    }
    else {
        mCamera.pitch = 0.45f;
        mCamera.distance = 26.0f;
        mRayTracer = new RayTracer("shaders/geodesic.comp", "shaders/quad.vert", "shaders/quad.frag");
    }


    mLastFrameTime = std::chrono::steady_clock::now();
}

BlackHoleVisualSim::~BlackHoleVisualSim() {
    delete mRenderer;
    delete mRayTracer;
}

void BlackHoleVisualSim::drawContents() {
    float dt = computeDeltaTime();
    updateCameraMovement(dt);
    syncBlackHoles(dt);

    if (mIs2DMode) {
        update2D(dt);
        render2D();
    }
    else {
        update3D(dt);
        render3D();
    }
}

void BlackHoleVisualSim::syncBlackHoles(float dt) {
    mBlackHoles.clear();

    auto makePos = [this](float a, float b) -> glm::vec3 {
        return mIs2DMode ? glm::vec3(a, b, 0.0f) : glm::vec3(a, 0.0f, b);
        };

    BlackHole bh1;
    bh1.rs = mParams.bh1Rs;
    bh1.ringColor = glm::vec4(1.0f, 0.55f, 0.15f, 0.9f);

    if (mParams.enableSecondBH && mParams.binaryOrbit) {
        if (!mParams.paused) {
            mBinaryAngle += mParams.binarySpeed * dt;
        }

        float totalMass = mParams.bh1Rs + mParams.bh2Rs;
        float r1 = mParams.binarySeparation * (mParams.bh2Rs / totalMass);
        float r2 = mParams.binarySeparation * (mParams.bh1Rs / totalMass);

        glm::vec3 dir = makePos(std::cos(mBinaryAngle), std::sin(mBinaryAngle));
        bh1.pos = -dir * r1;
        mBlackHoles.push_back(bh1);

        BlackHole bh2;
        bh2.rs = mParams.bh2Rs;
        bh2.pos = dir * r2;
        bh2.ringColor = glm::vec4(0.25f, 0.85f, 1.0f, 0.9f);
        mBlackHoles.push_back(bh2);
    }
    else {
        bh1.pos = makePos(mParams.bh1X, mParams.bh1Y);
        mBlackHoles.push_back(bh1);

        if (mParams.enableSecondBH) {
            BlackHole bh2;
            bh2.rs = mParams.bh2Rs;
            bh2.pos = makePos(mParams.bh2X, mParams.bh2Y);
            bh2.ringColor = glm::vec4(0.25f, 0.85f, 1.0f, 0.9f);
            mBlackHoles.push_back(bh2);
        }
    }
}


// ============================================================================
// Input Events
// ============================================================================
bool BlackHoleVisualSim::mouseButtonEvent(const Eigen::Vector2i& p, int button, bool down, int modifiers) {
    if (nanogui::Screen::mouseButtonEvent(p, button, down, modifiers)) return true;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        mDragging = down;
        return true;
    }
    return false;
}

bool BlackHoleVisualSim::mouseMotionEvent(const Eigen::Vector2i& p, const Eigen::Vector2i& rel, int button, int modifiers) {
    if (nanogui::Screen::mouseMotionEvent(p, rel, button, modifiers)) return true;

    if (mDragging) {
        mCamera.orbit(static_cast<float>(rel.x()), static_cast<float>(rel.y()));
        return true;
    }
    return false;
}

bool BlackHoleVisualSim::scrollEvent(const Eigen::Vector2i& p, const Eigen::Vector2f& rel) {
    if (nanogui::Screen::scrollEvent(p, rel)) return true;

    mCamera.zoom(rel.y());
    return true;
}

bool BlackHoleVisualSim::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (nanogui::Screen::keyboardEvent(key, scancode, action, modifiers)) return true;

    if (key >= 0 && key < static_cast<int>(mKeys.size())) {
        mKeys[key] = (action != GLFW_RELEASE);
    }

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_SPACE) mParams.paused = !mParams.paused;
        if (key == GLFW_KEY_C)     mCamera.reset();
        if (key == GLFW_KEY_R && mIs2DMode) mParams.resetRequested = true;
        if (key == GLFW_KEY_G && mControlWindow) {
            mControlWindow->setVisible(!mControlWindow->visible());
        }
    }
    return true;
}

// ============================================================================
// Frame & Camera Helpers
// ============================================================================
float BlackHoleVisualSim::computeDeltaTime() {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - mLastFrameTime).count();
    mLastFrameTime = now;
    return glm::min(dt, 0.05f);
}

void BlackHoleVisualSim::updateCameraMovement(float dt) {
    glm::vec3 moveInput(0.0f);
    if (mKeys[GLFW_KEY_W]) moveInput.z += 1.0f;
    if (mKeys[GLFW_KEY_S]) moveInput.z -= 1.0f;
    if (mKeys[GLFW_KEY_D]) moveInput.x += 1.0f;
    if (mKeys[GLFW_KEY_A]) moveInput.x -= 1.0f;
    if (mKeys[GLFW_KEY_E]) moveInput.y += 1.0f;
    if (mKeys[GLFW_KEY_Q]) moveInput.y -= 1.0f;

    if (glm::length(moveInput) > 0.0f) {
        mCamera.move(glm::normalize(moveInput), dt, mParams.cameraSpeed);
    }
}

// ============================================================================
// 2D Mode (Phase 1 & 2)
// ============================================================================
void BlackHoleVisualSim::spawnParallelRays(float startX) {
    mRays.clear();
    mRays.reserve(mParams.rayCount);

    float rad = glm::radians(mParams.rayAngleDeg);
    glm::vec3 dir(std::cos(rad), std::sin(rad), 0.0f);

    for (int i = 0; i < mParams.rayCount; ++i) {
        float t = (mParams.rayCount == 1) ? 0.5f : static_cast<float>(i) / (mParams.rayCount - 1);
        float y = glm::mix(-mParams.raySpreadY, mParams.raySpreadY, t);
        mRays.emplace_back(glm::vec3(startX, y, 0.0f), dir);
    }
}

void BlackHoleVisualSim::update2D(float dt) {
    if (mParams.resetRequested) {
        spawnParallelRays();
        mParams.resetRequested = false;
    }

    if (!mParams.paused) {
        for (auto& ray : mRays) {
            ray.step(dt * mParams.timeScale, mBlackHoles);
        }
    }
}

void BlackHoleVisualSim::render2D() {
    glClear(GL_DEPTH_BUFFER_BIT);
    float aspect = (mSize.y() > 0) ? static_cast<float>(mSize.x()) / mSize.y() : 1.0f;

    mRenderer->beginFrame(mCamera.getViewMatrix(), mCamera.getProjectionMatrix(aspect));

    for (const auto& bh : mBlackHoles) {
        mRenderer->drawCircle(
            bh.pos,
            bh.rs,
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            bh.ringColor
        );
    }

    for (const auto& ray : mRays) {
        mRenderer->drawFadingTrail(ray.trail, glm::vec3(1.0f, 0.9f, 0.45f));
        if (!ray.absorbed) {
            mRenderer->drawPoint(ray.pos, glm::vec4(1.0f, 1.0f, 0.85f, 1.0f), 4.0f);
        }
    }
}

// ============================================================================
// 3D Mode (Phase 3 Placeholders)
// ============================================================================
void BlackHoleVisualSim::update3D(float dt) {
    if (!mParams.paused) {
        mSimTime += dt * (mParams.timeScale * 0.25f);
    }
}


void BlackHoleVisualSim::render3D() {
    glClear(GL_DEPTH_BUFFER_BIT);
    float aspect = (mSize.y() > 0) ? static_cast<float>(mSize.x()) / mSize.y() : 1.0f;

    if (mParams.enableRayTracing && mRayTracer) {
        // GPU Ray Tracing handles the black hole, accretion disc, AND lensed spacetime grid
        mRayTracer->render(mFBSize.x(), mFBSize.y(), mCamera, mBlackHoles,
            mParams.showAccretionDisc, mParams.showSpacetimeGrid, mSimTime);
    }
    else {
        // Fallback rasterized rendering ONLY when GPU Ray Tracing is turned off
        mRenderer->beginFrame(mCamera.getViewMatrix(), mCamera.getProjectionMatrix(aspect));

        if (mParams.showSpacetimeGrid) {
            mRenderer->drawSpacetimeGrid(mBlackHoles);
        }

        for (const auto& bh : mBlackHoles) {
            mRenderer->drawSphere(bh.pos, bh.rs, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
    }
}


