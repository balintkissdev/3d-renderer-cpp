#include "camera.hpp"

#include "utils.hpp"

#include "glm/gtc/matrix_transform.hpp"

#include <algorithm>

namespace
{
// TODO: Make them configurable
constexpr float MOVEMENT_SPEED = 2.5f;
constexpr float LOOK_SENSITIVITY = 0.1f;

constexpr float ROTATION_Y_LIMIT = 89.0f;

/// Normalized mapping of positive Y axis in world coordinate space, always
/// pointing upwards in the viewport (x:0, y:1, z:0). Required to determine
/// the Right vector (mapping of positive X axis, done by GLM) when creating
/// the view matrix.
constexpr glm::vec3 UP_VECTOR{0.0f, 1.0f, 0.0f};
}  // namespace

Camera::Camera(const glm::vec3& position, const glm::vec2& rotation)
    : m_position(position)
    , m_rotation(rotation)
{
    // Avoid camera jump on first mouselook.
    updateDirection();
    static_cast<void>(calculateViewMatrix());
}

void Camera::moveForward(const float deltaTime)
{
    m_position += MOVEMENT_SPEED * m_direction * deltaTime;
}

void Camera::moveBackward(const float deltaTime)
{
    m_position -= MOVEMENT_SPEED * m_direction * deltaTime;
}

void Camera::strafeLeft(const float deltaTime)
{
    // If you don't normalize, you move fast or slow depending on camera
    // direction.
    m_position -= glm::normalize(glm::cross(m_direction, UP_VECTOR))
                * MOVEMENT_SPEED * deltaTime;
}

void Camera::strafeRight(const float deltaTime)
{
    m_position += glm::normalize(glm::cross(m_direction, UP_VECTOR))
                * MOVEMENT_SPEED * deltaTime;
}

void Camera::ascend(const float deltaTime)
{
    m_position += MOVEMENT_SPEED * UP_VECTOR * deltaTime;
}

void Camera::descend(const float deltaTime)
{
    m_position -= MOVEMENT_SPEED * UP_VECTOR * deltaTime;
}

void Camera::look(const float xOffset, const float yOffset)
{
    m_rotation.x += xOffset * LOOK_SENSITIVITY;
    // Wrap to keep rotation degrees displayed between 0 and 360 on debug UI
    utils::wrap(m_rotation.x, 0.0f, 359.9f);
    m_rotation.y += yOffset * LOOK_SENSITIVITY;
    // Avoid user to do a backflip
    m_rotation.y
        = glm::clamp(m_rotation.y, -ROTATION_Y_LIMIT, ROTATION_Y_LIMIT);

    if (m_rotation != m_cachedRotation)
    {
        updateDirection();
        m_cachedRotation = m_rotation;
    }
}

glm::mat4 Camera::calculateViewMatrix() const
{
    if (m_position == m_cachedPosition && m_direction == m_cachedDirection)
    {
        return m_cachedView;
    }

    m_cachedPosition = m_position;
    m_cachedDirection = m_direction;
    m_cachedView
        = glm::lookAtRH(m_position, m_position + m_direction, UP_VECTOR);
    return m_cachedView;
}

void Camera::updateDirection()
{
    const float rotationXRadian = glm::radians(m_rotation.x);
    const float rotationXSin = glm::sin(rotationXRadian);
    const float rotationXCos = glm::cos(rotationXRadian);
    const float rotationYRadian = glm::radians(m_rotation.y);
    const float rotationYSin = glm::sin(rotationYRadian);
    const float rotationYCos = glm::cos(rotationYRadian);
    m_direction.x = rotationXCos * rotationYCos;
    m_direction.y = rotationYSin;
    m_direction.z = rotationXSin * rotationYCos;
    m_direction = glm::normalize(m_direction);
}
