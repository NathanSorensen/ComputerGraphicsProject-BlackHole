#pragma once

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 target{ 0.0f, 0.0f, 0.0f };
    float distance = 22.0f;
    float yaw = 0.0f;   // Radians around Y axis (0 = +Z side)
    float pitch = 0.0f; // Radians elevation
    float fovDegrees = 45.0f;

    // Direction the camera is looking (always unit length, even when distance == 0)
    glm::vec3 getForward() const {
        float cosPitch = std::cos(pitch);
        return glm::normalize(glm::vec3(
            -cosPitch * std::sin(yaw),
            -std::sin(pitch),
            -cosPitch * std::cos(yaw)
        ));
    }

    glm::vec3 getPosition() const {
        return target - getForward() * distance;
    }

    glm::mat4 getViewMatrix() const {
        glm::vec3 eye = getPosition();
        return glm::lookAt(eye, eye + getForward(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 getProjectionMatrix(float aspectRatio) const {
        return glm::perspective(glm::radians(fovDegrees), aspectRatio, 0.1f, 500.0f);
    }

    void orbit(float deltaX, float deltaY, float sensitivity = 0.005f) {
        yaw -= deltaX * sensitivity;
        pitch += deltaY * sensitivity;
        constexpr float maxPitch = 1.55f;
        pitch = std::clamp(pitch, -maxPitch, maxPitch);
    }

    void zoom(float scrollOffset, float sensitivity = 1.5f) {
        // Clamping to 0.0f transitions smoothly into 1st-person mode when fully zoomed in
        distance = std::clamp(distance - scrollOffset * sensitivity, 0.0f, 150.0f);
    }

    void move(const glm::vec3& localInput, float deltaTime, float speed = 12.0f) {
        glm::vec3 forward = getForward();
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        target += (forward * localInput.z + right * localInput.x + up * localInput.y) * speed * deltaTime;
    }

    void reset(bool is2DMode = true) {
        target = glm::vec3(0.0f, 0.0f, 0.0f);
        yaw = 0.0f;
        pitch = is2DMode ? 0.0f : 0.45f;
        distance = is2DMode ? 22.0f : 26.0f;
    }
};
