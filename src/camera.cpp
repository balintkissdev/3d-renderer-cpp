#include "camera.hpp"

#include "utils.hpp"

#include "glm/gtc/matrix_transform.hpp"

#include <algorithm>

namespace
{
// TODO: Make them configurable
constexpr float MOVEMENT_SPEED = 2.5F;
constexpr float LOOK_SENSITIVITY = 0.1F;

constexpr float ROTATION_Y_LIMIT = 89.0F;

/// Normalized mapping of positive Y axis in world coordinate space, always
/// pointing upwards in the viewport (x:0, y:1, z:0). Required to determine
/// the Right vector (mapping of positive X axis, done by GLM) when creating
/// the view matrix.
constexpr glm::vec3 UP_VECTOR{0.0F, 1.0F, 0.0F};
}  // namespace

Camera::Camera(const glm::vec3& position, const glm::vec2& rotation)
    : position_(position)
    , rotation_(rotation)
{
    // Avoid camera jump on first mouselook.
    updateDirection();
    static_cast<void>(calculateViewMatrix());
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
    rotation_.y = glm::clamp(rotation_.y, -ROTATION_Y_LIMIT, ROTATION_Y_LIMIT);

    if (rotation_ != cachedRotation_)
    {
        updateDirection();
        cachedRotation_ = rotation_;
    }
}

glm::mat4 Camera::calculateViewMatrix() const
{
    if (position_ == cachedPosition_ && direction_ == cachedDirection_)
    {
        return cachedView_;
    }

    cachedPosition_ = position_;
    cachedDirection_ = direction_;
    cachedView_ = glm::lookAt(position_, position_ + direction_, UP_VECTOR);
    return cachedView_;
}

void Camera::updateDirection()
{
    const float rotationXRadian = glm::radians(rotation_.x);
    const float rotationXSin = glm::sin(rotationXRadian);
    const float rotationXCos = glm::cos(rotationXRadian);
    const float rotationYRadian = glm::radians(rotation_.y);
    const float rotationYSin = glm::sin(rotationYRadian);
    const float rotationYCos = glm::cos(rotationYRadian);
    direction_.x = rotationXCos * rotationYCos;
    direction_.y = rotationYSin;
    direction_.z = rotationXSin * rotationYCos;
    direction_ = glm::normalize(direction_);
}
