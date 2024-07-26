#include "camera.h"

#include "utils.h"

#include "glm/gtc/matrix_transform.hpp"

#include <algorithm>

namespace
{
// TODO: Make them configurable
constexpr float MOVEMENT_SPEED = 2.5F;
constexpr float LOOK_SENSITIVITY = 0.1F;
}  // namespace

const glm::vec3 Camera::UP_VECTOR{0.0F, 1.0F, 0.0F};

Camera::Camera(const glm::vec3& position, const glm::vec2& rotation)
    : position_(position)
    , rotation_(rotation)
{
    // Avoid camera jump on first mouselook.
    updateDirection();
}

void Camera::moveForward(const float deltaTime)
{
    position_ += MOVEMENT_SPEED * direction_ * deltaTime;
}

void Camera::moveBackward(const float deltaTime)
{
    position_ -= MOVEMENT_SPEED * direction_ * deltaTime;
}

void Camera::strafeLeft(const float deltaTime)
{
    // If you don't normalize, you move fast or slow depending on camera
    // direction.
    position_ -= glm::normalize(glm::cross(direction_, UP_VECTOR))
               * MOVEMENT_SPEED * deltaTime;
}

void Camera::strafeRight(const float deltaTime)
{
    position_ += glm::normalize(glm::cross(direction_, UP_VECTOR))
               * MOVEMENT_SPEED * deltaTime;
}

void Camera::ascend(const float deltaTime)
{
    position_ += MOVEMENT_SPEED * UP_VECTOR * deltaTime;
}

void Camera::descend(const float deltaTime)
{
    position_ -= MOVEMENT_SPEED * UP_VECTOR * deltaTime;
}

void Camera::look(const float xOffset, const float yOffset)
{
    rotation_.x += xOffset * LOOK_SENSITIVITY;
    // Wrap to keep rotation degrees displayed between 0 and 360 on debug UI
    utils::wrap(rotation_.x, 0.0F, 359.9F);
    rotation_.y += yOffset * LOOK_SENSITIVITY;
    // Avoid user to do a backflip
    rotation_.y = std::clamp(rotation_.y, -89.0F, 89.0F);

    updateDirection();
}

glm::mat4 Camera::calculateViewMatrix() const
{
    return glm::lookAt(position_, position_ + direction_, UP_VECTOR);
}

void Camera::updateDirection()
{
    direction_.x = std::cos(glm::radians(rotation_.x))
                 * std::cos(glm::radians(rotation_.y));
    direction_.y = std::sin(glm::radians(rotation_.y));
    direction_.z = std::sin(glm::radians(rotation_.x))
                 * std::cos(glm::radians(rotation_.y));
    direction_ = glm::normalize(direction_);
}
