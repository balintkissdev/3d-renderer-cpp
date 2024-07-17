#include "camera.h"

#include "utils.h"

#include "glm/gtc/matrix_transform.hpp"

const glm::vec3 Camera::UP_VECTOR{0.0f, 1.0f, 0.0f};

Camera::Camera(const glm::vec3& position, const glm::vec2& rotation)
    : rotation_(rotation)
    , position_(position)
{
    // Calculate front vector on initialization to avoid jumping camera on first
    // mouselook
    updateDirection();
}

void Camera::moveForward(const float deltaTime)
{
    position_ += movementSpeed * front_ * deltaTime;
}

void Camera::moveBackward(const float deltaTime)
{
    position_ -= movementSpeed * front_ * deltaTime;
}

void Camera::strafeLeft(const float deltaTime)
{
    // If you don't normalize, you move fast or slow depending on camera
    // direction.
    position_ -= glm::normalize(glm::cross(front_, UP_VECTOR)) * movementSpeed
               * deltaTime;
}

void Camera::strafeRight(const float deltaTime)
{
    position_ += glm::normalize(glm::cross(front_, UP_VECTOR)) * movementSpeed
               * deltaTime;
}

void Camera::ascend(const float deltaTime)
{
    position_ += movementSpeed * UP_VECTOR * deltaTime;
}

void Camera::descend(const float deltaTime)
{
    position_ -= movementSpeed * UP_VECTOR * deltaTime;
}

void Camera::look(const float xOffset, const float yOffset)
{
    rotation_.x += xOffset * lookSensitivity;
    utils::wrap(rotation_.x, 0.0f, 359.9f);
    rotation_.y += yOffset * lookSensitivity;
    utils::clamp(rotation_.y, -89.0f, 89.0f);

    updateDirection();
}

glm::mat4 Camera::makeViewMatrix() const
{
    return glm::lookAt(position_, position_ + front_, UP_VECTOR);
}

glm::vec3 Camera::position() const
{
    return position_;
}

glm::vec2 Camera::rotation() const
{
    return rotation_;
}

void Camera::updateDirection()
{
    glm::vec3 direction;
    direction.x = std::cos(glm::radians(rotation_.x))
                * std::cos(glm::radians(rotation_.y));
    direction.y = std::sin(glm::radians(rotation_.y));
    direction.z = std::sin(glm::radians(rotation_.x))
                * std::cos(glm::radians(rotation_.y));
    front_ = glm::normalize(direction);
}
