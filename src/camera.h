#pragma once

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

class Camera
{
public:
    Camera(const glm::vec3 &position, const glm::vec2 &rotation);

    void moveForward(const float deltaTime);
    void moveBackward(const float deltaTime);
    void strafeLeft(const float deltaTime);
    void strafeRight(const float deltaTime);
    void ascend(const float deltaTime);
    void descend(const float deltaTime);
    void look(const float xOffset, const float yOffset);

    [[nodiscard]] glm::mat4 makeViewMatrix() const;
    [[nodiscard]] glm::vec3 position() const;
    [[nodiscard]] glm::vec2 rotation() const;

private:
    static const glm::vec3 UP_VECTOR;

    glm::vec2 rotation_;  // rotation_.x is also known as yaw, while rotation_.y
                          // is pitch
    glm::vec3 position_;
    glm::vec3 front_;

    const float movementSpeed_ = 2.5F;
    const float lookSensitivity_ = 0.1F;

    void updateDirection();
};
